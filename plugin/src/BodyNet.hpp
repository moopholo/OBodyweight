#pragma once

// ── BodyNet: OBW's own tiny neural body generator ────────────────────────────────────────────────
// A learned replacement for the hand-written slider derivation. Trained offline (tools/bodynet) on
// the curated bank of 2304 hand-made BodySlide presets; the shipped artefact is body.obwnet, an
// ~86k-parameter MLP. Measured against the real presets it reproduces their JOINT distribution far
// better than the procedural rules (correlation distance 0.053 vs 0.188, marginals 1.8 vs 16.1) at
// equal variety and equal mesh safety - see tools/bodynet/BODYNET.md for the full comparison.
//
// WHY OUR OWN ENGINE (and not the Myelin runtime this started from): Myelin ships only its DLL - the
// client header and the recipe spec live in a repository that is not public, so a client could only be
// written by guessing struct layouts. The net is small enough that evaluating it ourselves is a few
// dozen lines with NO dependency, which also keeps OBW standalone: a public mod should not hand its
// users a hard dependency on a third-party runtime.
//
// DETERMINISM IS PRESERVED. The noise vector is derived from the actor's seed, so the same NPC still
// gets the same body across saves, exactly like the procedural path.
//
// THE NET PROPOSES, OBW DISPOSES. A VAE decoder has unbounded tails (a raw draw produced Breasts=177,
// Belly=-50), so every output passes through the SAME per-slider ceilings the procedural path uses
// before it reaches SKEE. The learned generator can never break a mesh.
//
// No CommonLibSSE / game types here on purpose: plugin/sim/sim_bodynet.cpp includes this directly to
// run the parity test against the Python reference on the host.

#include <cstdint>
#include <string>
#include <vector>

namespace OBW::BodyNet {

// Three independent models: female CBBE 3BA and BHUNP (49 sliders, 24 archetypes each), plus male HIMBO
// (29 sliders, 12 archetypes). They are loaded, gated and validated independently, so a missing or weak
// model for one body never affects the others.
enum class Slot : int {
    Female = 0,       // CBBE 3BA (also the safe fallback when the base body is ambiguous)
    FemaleBHUNP = 1,  // BHUNP-specific fine-tune
    Male = 2,         // HIMBO
    Count = 3
};


// Load body.obwnet. Returns false (and leaves the feature off) on any malformed or missing file -
// every refusal names the reason. Safe to call more than once.
bool Load(Slot a_slot, const std::string& a_path, std::string& a_error);

// True once a model is loaded; every call site must gate on this and fall back to the procedural path.
bool Available(Slot a_slot);

// Slider names the model produces, in output order (self-describing file - callers map by NAME,
// never by index, so a re-trained model with a different slider set can't silently scramble values).
const std::vector<std::string>& SliderNames(Slot a_slot);

int ArchetypeCount(Slot a_slot);

// Generate one body. a_archetype = OBW's archetype index (the model's embedding row), a_seed = the
// actor seed (drives the deterministic noise), a_weight01 = the NPC's simulated weight 0..1.
// a_out is resized to SliderNames().size(), values on the usual 0-100 slider scale, already clamped.
// a_natural01: 1 = a body from the modest region of real hand-made presets, 0 = from the striking one.
// a_shape01:   the "Natural women / Curvy women" pole (female) or lean<->bulky "Male build" (male).
// a_tone01:    muscle definition - the "Athletic women" dial (female) / male muscle.
// All three are CONDITIONING inputs learned from the corpus, never transforms applied on top: the net
// reproduces how real preset authors express each axis, which a multiplier on a finished body cannot.
// It is a CONDITIONING input, never a cap - OBW maps its existing fantasy roll onto it, so the same
// NPCs that would have been "fantasy" today come out as the exuberant ones, at the same frequency.
void Generate(Slot a_slot, int a_archetype, std::uint32_t a_seed, float a_weight01, float a_natural01,
              float a_shape01, float a_tone01, std::vector<float>& a_out);

// Apply the existing rare unusual-breast trait to a generated female body. Kept in this game-free
// module so the host simulator can verify determinism, mutual exclusion and mesh-safe ranges.
void ApplyUnusualBreasts(Slot a_slot, std::uint32_t a_seed, float a_ratio, std::vector<float>& a_values);

// Raw forward pass with an EXPLICIT noise vector - used only by the parity test (the shipped path
// derives z from the seed). a_z must have ZDim() entries; output is NOT clamped.
// Per-slider mesh-safety ceiling on the 0-100 BodySlide scale (mirrors WeightManager's VolumeCeiling).
// Exposed so the integration boundary can assert against the SAME table Generate clamps with, instead
// of a hand-picked constant - see the sanity gate in WeightManager::BuildNeuralBody.
float Ceiling100(const std::string& a_slider);

int  ZDim(Slot a_slot);
void Forward(Slot a_slot, int a_archetype, const float* a_z, float a_weight01, float a_natural01,
             float a_shape01, float a_tone01, std::vector<float>& a_out);

}  // namespace OBW::BodyNet
