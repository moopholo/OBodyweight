// BodyNet (upstream 1.6.0's learned body generator) is game-free, so the shipped .obwnet models can be
// exercised on the host: they must load, be deterministic per actor seed (same NPC -> same body across
// saves), and never leave the per-slider mesh-safety ceilings the procedural path also uses.
#include <doctest/doctest.h>

#include "BodyNet.hpp"

#include <cstdint>
#include <string>
#include <vector>

#ifndef OBW_MODEL_DIR
#define OBW_MODEL_DIR "SKSE/Plugins/OBodyNGWeight"   // relative to the repo root
#endif

namespace BN = OBW::BodyNet;

namespace {
bool LoadSlot(BN::Slot slot, const char* file) {
    std::string err;
    const bool ok = BN::Load(slot, std::string(OBW_MODEL_DIR) + "/" + file, err);
    if (!ok) MESSAGE("BodyNet load failed for ", file, ": ", err);
    return ok;
}

void CheckModel(BN::Slot slot, int expectArchetypes) {
    REQUIRE(BN::Available(slot));
    CHECK(BN::ArchetypeCount(slot) == expectArchetypes);
    const auto& names = BN::SliderNames(slot);
    REQUIRE_FALSE(names.empty());

    std::vector<float> a, b;
    for (std::uint32_t seed : { 1u, 0xDEADBEEFu, 0x00012345u, 0xFF0A1B2Cu }) {
        for (int arch = 0; arch < expectArchetypes; ++arch) {
            for (float w01 : { 0.0f, 0.5f, 1.0f }) {
                BN::Generate(slot, arch, seed, w01, 0.2f, 0.5f, 0.7f, a);
                BN::Generate(slot, arch, seed, w01, 0.2f, 0.5f, 0.7f, b);
                REQUIRE(a.size() == names.size());
                CHECK(a == b);                                   // deterministic per seed
                for (std::size_t i = 0; i < a.size(); ++i) {
                    CHECK(a[i] >= 0.0f);
                    CHECK(a[i] <= BN::Ceiling100(names[i]));     // inside the mesh-safety rails
                }
            }
        }
    }
    // Different seeds give different bodies (the noise really is driven by the seed).
    BN::Generate(slot, 0, 1u, 0.5f, 0.5f, 0.5f, 0.5f, a);
    BN::Generate(slot, 0, 2u, 0.5f, 0.5f, 0.5f, 0.5f, b);
    CHECK(a != b);
}
}  // namespace

TEST_CASE("BodyNet F3BA: loads, deterministic per seed, within Ceiling100") {
    REQUIRE(LoadSlot(BN::Slot::Female, "bodynet_f3ba.obwnet"));
    CheckModel(BN::Slot::Female, 24);   // 24 = OBW's female archetype table (kArchetypes)
}

TEST_CASE("BodyNet BHUNP: loads, deterministic per seed, within Ceiling100") {
    REQUIRE(LoadSlot(BN::Slot::FemaleBHUNP, "bodynet_fbhunp.obwnet"));
    CheckModel(BN::Slot::FemaleBHUNP, 24);
}

TEST_CASE("BodyNet HIMBO: loads, deterministic per seed, within Ceiling100") {
    REQUIRE(LoadSlot(BN::Slot::Male, "bodynet_mhimbo.obwnet"));
    CheckModel(BN::Slot::Male, 12);     // 12 = OBW's male archetype table (kMaleArchetypes)
}

TEST_CASE("BodyNet: unusual-breast trait stays deterministic and inside the rails") {
    REQUIRE(LoadSlot(BN::Slot::Female, "bodynet_f3ba.obwnet"));
    const auto& names = BN::SliderNames(BN::Slot::Female);
    std::vector<float> a, b;
    for (std::uint32_t seed = 1; seed < 200; ++seed) {
        BN::Generate(BN::Slot::Female, static_cast<int>(seed % 24), seed, 0.6f, 0.3f, 0.5f, 0.3f, a);
        b = a;
        BN::ApplyUnusualBreasts(BN::Slot::Female, seed, 1.0f, a);
        BN::ApplyUnusualBreasts(BN::Slot::Female, seed, 1.0f, b);
        CHECK(a == b);
        for (std::size_t i = 0; i < a.size(); ++i) CHECK(a[i] <= BN::Ceiling100(names[i]));
    }
}

TEST_CASE("BodyNet: missing model is refused, not crashed") {
    std::string err;
    CHECK_FALSE(BN::Load(BN::Slot::Male, "does/not/exist.obwnet", err));
    CHECK_FALSE(err.empty());
    CHECK_FALSE(BN::Available(BN::Slot::Male));
    std::vector<float> out{ 1.0f };
    BN::Generate(BN::Slot::Male, 0, 1u, 0.5f, 0.5f, 0.5f, 0.5f, out);
    CHECK(out.empty());
}
