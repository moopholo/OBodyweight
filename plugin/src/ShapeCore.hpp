#pragma once
// ShapeCore — the game-free (no RE::/SKSE) core of OBW's body generation, split out
// so it can be unit-tested without the Skyrim runtime. WeightManager keeps the thin
// glue that turns an RE::Actor into the primitives these functions take (a seed from
// the FormID, a RaceClass from the race). Grow this module as more pure logic moves out.
#include <cstdint>
#include <string>

namespace OBW {

// Coarse race grouping that biases archetype selection (see the per-race weight tables).
enum class RaceClass : std::uint8_t {
    kNeutral = 0, kNord, kImperial, kBreton, kRedguard, kOrc,
    kAltmer, kBosmer, kDunmer, kKhajiit, kArgonian, kElder
};

// Classify a LOWERCASED race editorID/name into a RaceClass. Order matters: more specific
// tokens (e.g. "highelf") are tested before generic ones ("elf"); "elder" is tested first so
// aged variants group as Elder regardless of their underlying race. Returns kNeutral for
// empty/unknown strings (→ legacy uniform distribution, no regression).
RaceClass ClassifyRaceStr(const std::string& s);

}  // namespace OBW
