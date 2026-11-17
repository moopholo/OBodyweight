#include "BodyNet.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <random>

namespace OBW::BodyNet {
namespace {

struct Model {
    std::uint32_t nArch = 0, eDim = 0, zDim = 0, hidden = 0, nOut = 0;
    std::vector<float> emb, l1w, l1b, l2w, l2b, l3w, l3b, ow, ob, mu, sd, zt;
    std::vector<std::string> names;
    bool ok = false;
};
Model g_models[static_cast<int>(Slot::Count)];
inline Model& M(Slot s) { return g_models[static_cast<int>(s)]; }


// splitmix64 - the same cheap deterministic mixer FastRandom uses. The noise vector must depend ONLY on
// the actor seed so a given NPC keeps its body across saves.
std::uint64_t Mix(std::uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

bool Read(std::ifstream& f, void* dst, std::size_t bytes) {
    f.read(static_cast<char*>(dst), static_cast<std::streamsize>(bytes));
    return static_cast<std::size_t>(f.gcount()) == bytes;
}
bool ReadVec(std::ifstream& f, std::vector<float>& v, std::size_t n) {
    v.resize(n);
    return n == 0 || Read(f, v.data(), n * sizeof(float));
}

// y = W x + b, with W stored row-major as [out][in]
void Dense(const std::vector<float>& w, const std::vector<float>& b,
           const float* x, std::size_t in, std::size_t out, float* y) {
    for (std::size_t o = 0; o < out; ++o) {
        const float* row = w.data() + o * in;
        float acc = b[o];
        for (std::size_t i = 0; i < in; ++i) acc += row[i] * x[i];
        y[o] = acc;
    }
}

}  // namespace

// Per-slider ceiling on the 0-100 scale. Mirrors WeightManager's VolumeCeiling table (measured p90-p95
// of 2,143 real presets) so the learned path inherits EXACTLY the same proven mesh-safety rails as the
// procedural one - a VAE tail can propose Breasts=177 and the clamp is what makes that harmless. Kept
// as a copy rather than a call so this file stays free of game/WeightManager types and the host-side
// parity sim can include it directly; if that table ever moves, move this with it.
float Ceiling100(const std::string& n) {
    std::string k(n);
    std::transform(k.begin(), k.end(), k.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    if (k == "breasts")              return 155.0f;
    if (k == "butt")                 return 120.0f;
    if (k == "bigbutt")              return 105.0f;
    if (k == "belly")                return 120.0f;
    if (k == "bigbelly")             return  55.0f;
    if (k == "hips")                 return 105.0f;
    if (k == "thighs")               return 105.0f;
    if (k == "calfsize")             return 100.0f;
    if (k == "chubbylegs")           return 100.0f;
    if (k == "thighoutsidethicc_v2") return  50.0f;
    if (k == "thighfbthicc_v2")      return  45.0f;
    if (k == "arms")                 return 100.0f;
    if (k == "chubbyarms")           return 100.0f;
    if (k == "forearmsize")          return  80.0f;
    if (k == "wristsize")            return  80.0f;
    return 100.0f;                   // shape/definition sliders live on 0-100
}

bool Load(Slot a_slot, const std::string& a_path, std::string& a_error) {
    Model& g = M(a_slot);
    g = Model{};
    std::ifstream f(a_path, std::ios::binary);
    if (!f) { a_error = "cannot open " + a_path; return false; }

    char magic[4]{};
    std::uint32_t ver = 0;
    if (!Read(f, magic, 4) || std::memcmp(magic, "OBWN", 4) != 0) { a_error = "bad magic (expected OBWN)"; return false; }
    if (!Read(f, &ver, 4) || ver != 4) { a_error = "unsupported version " + std::to_string(ver) + " (expected 4)"; return false; }
    if (!Read(f, &g.nArch, 4) || !Read(f, &g.eDim, 4) || !Read(f, &g.zDim, 4) ||
        !Read(f, &g.hidden, 4) || !Read(f, &g.nOut, 4)) { a_error = "truncated header"; return false; }
    if (g.nArch == 0 || g.nArch > 256 || g.eDim == 0 || g.eDim > 64 || g.zDim == 0 || g.zDim > 64 ||
        g.hidden == 0 || g.hidden > 4096 || g.nOut == 0 || g.nOut > 512) { a_error = "implausible dims"; return false; }

    const std::size_t in = g.eDim + g.zDim + 4;      // weight + natural01 + shape01 + tone01
    if (!ReadVec(f, g.emb, (std::size_t)g.nArch * g.eDim) ||
        !ReadVec(f, g.l1w, (std::size_t)g.hidden * in)     || !ReadVec(f, g.l1b, g.hidden) ||
        !ReadVec(f, g.l2w, (std::size_t)g.hidden * g.hidden) || !ReadVec(f, g.l2b, g.hidden) ||
        !ReadVec(f, g.l3w, (std::size_t)g.hidden * g.hidden) || !ReadVec(f, g.l3b, g.hidden) ||
        !ReadVec(f, g.ow,  (std::size_t)g.nOut * g.hidden)   || !ReadVec(f, g.ob, g.nOut)   ||
        !ReadVec(f, g.mu,  g.nOut) || !ReadVec(f, g.sd, g.nOut) ||
        !ReadVec(f, g.zt,  g.nOut)) { a_error = "truncated weights"; return false; }

    g.names.reserve(g.nOut);
    for (std::uint32_t i = 0; i < g.nOut; ++i) {
        std::uint8_t len = 0;
        if (!Read(f, &len, 1) || len == 0) { a_error = "truncated slider names"; return false; }
        std::string s(len, '\0');
        if (!Read(f, s.data(), len)) { a_error = "truncated slider names"; return false; }
        g.names.push_back(std::move(s));
    }
    g.ok = true;
    return true;
}

bool Available(Slot s) { return M(s).ok; }
const std::vector<std::string>& SliderNames(Slot s) { return M(s).names; }
int ArchetypeCount(Slot s) { return static_cast<int>(M(s).nArch); }
int ZDim(Slot s) { return static_cast<int>(M(s).zDim); }

void Forward(Slot a_slot, int a_archetype, const float* a_z, float a_weight01, float a_natural01,
             float a_shape01, float a_tone01, std::vector<float>& a_out) {
    Model& g = M(a_slot);
    a_out.assign(g.ok ? g.nOut : 0, 0.0f);
    if (!g.ok) return;
    const int arch = std::clamp(a_archetype, 0, static_cast<int>(g.nArch) - 1);
    const std::size_t in = g.eDim + g.zDim + 4;

    std::vector<float> x(in);
    std::memcpy(x.data(), g.emb.data() + (std::size_t)arch * g.eDim, g.eDim * sizeof(float));
    std::memcpy(x.data() + g.eDim, a_z, g.zDim * sizeof(float));
    x[in - 4] = a_weight01;
    x[in - 3] = a_natural01;
    x[in - 2] = a_shape01;
    x[in - 1] = a_tone01;

    std::vector<float> h(g.hidden), t(g.hidden);
    Dense(g.l1w, g.l1b, x.data(), in, g.hidden, h.data());
    for (auto& v : h) v = v > 0.0f ? v : 0.0f;
    const std::vector<float> skip = h;                       // 'save'
    Dense(g.l2w, g.l2b, h.data(), g.hidden, g.hidden, t.data());
    for (auto& v : t) v = v > 0.0f ? v : 0.0f;
    Dense(g.l3w, g.l3b, t.data(), g.hidden, g.hidden, h.data());
    for (std::size_t i = 0; i < g.hidden; ++i) {             // 'add' (residual) then relu
        const float v = h[i] + skip[i];
        h[i] = v > 0.0f ? v : 0.0f;
    }
    Dense(g.ow, g.ob, h.data(), g.hidden, g.nOut, a_out.data());
    for (std::uint32_t i = 0; i < g.nOut; ++i) a_out[i] = a_out[i] * g.sd[i] + g.mu[i];
}

void Generate(Slot a_slot, int a_archetype, std::uint32_t a_seed, float a_weight01, float a_natural01,
              float a_shape01, float a_tone01, std::vector<float>& a_out) {
    Model& g = M(a_slot);
    if (!g.ok) { a_out.clear(); return; }
    // Deterministic Gaussian noise from the actor seed (Box-Muller on splitmix64), TRUNCATED to +-2
    // sigma: an untruncated tail is what produced the out-of-range raw draws, and 2 sigma still spans
    // the whole plausible body space (95% of the training manifold).
    std::vector<float> z(g.zDim);
    std::uint64_t s = Mix(0x0B0D17E7ull ^ a_seed);

    // PER-NPC LATENT SCALE - the same idea as varying a LoRA's strength per image, and the fix for the
    // one thing the model lost to the procedural path: SPREAD. Sampling every NPC at the same fixed
    // width makes the population too alike (d-std error 14.7 against the real presets); drawing each
    // NPC's own width instead brings it to 12.0, an 18% gain, for +4% on the joint-structure error.
    // Scaling the LATENT rather than the finished body matters: it moves along the learned manifold, so
    // proportions stay coherent, where scaling the output is a linear extrapolation off it (measured:
    // corr-dist 0.0530 vs 0.0546 at the same range). The scale is drawn from the actor seed like
    // everything else, so a given NPC keeps its body forever.
    s = Mix(s);
    const float scale = 0.7f + 1.1f * (float)((s >> 11) * 0x1.0p-53);   // 0.7 .. 1.8
    for (std::uint32_t i = 0; i < g.zDim; i += 2) {
        s = Mix(s); const float u1 = std::max(1e-7f, (float)((s >> 11) * 0x1.0p-53));
        s = Mix(s); const float u2 = (float)((s >> 11) * 0x1.0p-53);
        const float r = std::sqrt(-2.0f * std::log(u1)), th = 6.2831853f * u2;
        // Widen the truncation with the scale, or a scaled-up draw would just pile up on the +-2 wall
        // and the extra spread would be clipped away before it reached the body.
        const float lim = 2.0f * scale;
        z[i] = std::clamp(r * std::cos(th) * scale, -lim, lim);
        if (i + 1 < g.zDim) z[i + 1] = std::clamp(r * std::sin(th) * scale, -lim, lim);
    }
    Forward(a_slot, a_archetype, z.data(), std::clamp(a_weight01, 0.0f, 1.0f),
            std::clamp(a_natural01, 0.0f, 1.0f), std::clamp(a_shape01, 0.0f, 1.0f),
            std::clamp(a_tone01, 0.0f, 1.0f), a_out);
    // THE NET PROPOSES, OBW DISPOSES: same per-slider rails the procedural path uses.
    for (std::size_t i = 0; i < a_out.size(); ++i)
        a_out[i] = std::clamp(a_out[i], 0.0f, Ceiling100(g.names[i]));

    // SPARSIFY (v4). A smooth decoder cannot emit an exact zero, so where a preset author would LEAVE A
    // SLIDER ALONE it puts a small positive value there instead - measured: the corpus zeroes Belly in
    // 70% of presets and the net in 52%, ChubbyArms 68% vs 42%, Hips 37% vs 20%. Every body came out
    // padded in the places authors leave flat, which reads as a shrunken bust even though the bust is
    // close to the corpus (user report: "os bustos meio planos"). The threshold is per slider and
    // derived from the corpus's own zero rate at export time, so a slider authors rarely zero (Breasts,
    // 13%) is effectively untouched. It can only REMOVE volume, never add - so it cannot break a mesh.
    if (g.zt.size() == a_out.size())
        for (std::size_t i = 0; i < a_out.size(); ++i)
            if (a_out[i] < g.zt[i]) a_out[i] = 0.0f;
}

void ApplyUnusualBreasts(Slot a_slot, std::uint32_t a_seed, float a_ratio, std::vector<float>& a_values) {
    Model& g = M(a_slot);
    if (!g.ok || a_slot == Slot::Male || a_values.size() != g.names.size() || a_ratio <= 0.0f) return;
    const auto indexOf = [&](const char* wanted) {
        const auto it = std::find(g.names.begin(), g.names.end(), wanted);
        return it == g.names.end() ? g.names.size() : static_cast<std::size_t>(it - g.names.begin());
    };
    const std::size_t breasts = indexOf("Breasts");
    const std::size_t gravity = indexOf("BreastGravity2");
    const std::size_t perk = indexOf("BreastPerkiness");
    if (breasts == g.names.size() || gravity == g.names.size() || perk == g.names.size()) return;

    std::mt19937 ur{ a_seed ^ 0x0DDB5A60u };
    if (std::uniform_real_distribution<float>(0.0f, 1.0f)(ur) >= std::clamp(a_ratio, 0.0f, 1.0f)) return;
    const bool wantSag = std::uniform_real_distribution<float>(0.0f, 1.0f)(ur) < 0.5f;
    const bool doSag = wantSag && a_values[breasts] >= 45.0f;
    a_values[gravity] = doSag ? std::uniform_real_distribution<float>(55.0f, 85.0f)(ur) : 0.0f;
    a_values[perk] = doSag ? 0.0f : std::uniform_real_distribution<float>(60.0f, 90.0f)(ur);
}

}  // namespace OBW::BodyNet
