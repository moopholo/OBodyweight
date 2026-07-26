#pragma once
// ShapeCore — the game-free (no RE::/SKSE) core of OBW's body generation, split out
// so it can be unit-tested without the Skyrim runtime. WeightManager keeps the thin
// glue that turns an RE::Actor into the primitives these functions take (a seed from
// the FormID, a RaceClass from the race). Grow this module as more pure logic moves out.
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

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

// Canonical, stable, lowercase group name for a built-in RaceClass ("nord","orc","elder","neutral"…).
// Used both as the RaceRules map key and as an INI section name, so the two never drift.
std::string_view RaceClassName(RaceClass rc);

// ── Race / group body rules ──────────────────────────────────────────────────────────────────────
// A "group" is a named bucket of races (the built-in RaceClass names, plus any user-defined custom
// groups) that carries body-shape rules: per-archetype weight multipliers, a hard-exclude set, and a
// frame (body-volume) bias. This is the data model behind race coherence — moved here so the tables,
// the resolution, and the multiplier math are all unit-testable, and so users can retune them from an
// INI without recompiling.

struct GroupRule {
    // Archetype NAME → weight multiplier (default 1.0 for any archetype not listed). Keyed by name so
    // the tables survive a reorder of the archetype vectors.
    std::unordered_map<std::string, float> mult;
    // Archetype NAMES this group NEVER rolls. A hard exclude forces the effective weight to 0
    // ABSOLUTELY — independent of coherence strength — so it is a true safety rail (e.g. Elders never
    // get "sexy" bodies even with coherence turned off). Distinct from a 0.0 multiplier, which only
    // takes effect while coherence is on.
    std::unordered_set<std::string> exclude;
    float frameBias = 0.0f;   // additive body-VOLUME bias (not height), lerped by coherence strength
};

struct RaceRules {
    std::unordered_map<std::string, GroupRule> groups;  // group name (lowercase) → rule
    // Ordered (substring → group name) rules that classify a modded race string into a CUSTOM group.
    // Checked in order before the built-in RaceClass, so a mod's races can get bespoke rules. The
    // substrings are matched against the LOWERCASED race editorID/name.
    std::vector<std::pair<std::string, std::string>> customMatch;
};

// Build the shipped default rules for one sex (female or male): the same per-race archetype multipliers
// OBW has always used, PLUS the new default hard-excludes (Elders never roll the "sexy" archetypes).
RaceRules DefaultRaceRules(bool male);

// Resolve a LOWERCASED race string to a group name: custom match rules win (checked in order), else the
// built-in RaceClass name, else "neutral".
std::string ResolveGroup(const std::string& raceLower, const RaceRules& rules);

// Effective archetype weight multiplier for an already-resolved group. A hard exclude ALWAYS returns 0
// (absolute, ignores strength). Otherwise the group's multiplier (default 1.0) is lerped by `strength`
// (0 → 1.0 = no biasing, identical to legacy; 1 → the full raw multiplier). So coherence tunes flavor
// while excludes are absolute. An unknown group or archetype → strength-independent 1.0.
float GroupArchMult(const std::string& group, const std::string& archetype,
                    const RaceRules& rules, float strength);

// Frame (body-volume) bias for an already-resolved group, lerped by strength (0 → 0). Unknown group → 0.
float GroupFrameBias(const std::string& group, const RaceRules& rules, float strength);

// Overlay one parsed INI section onto the rule sets. PURE (no Win32) so the section→rule mapping is
// tested directly; WeightManager only enumerates the raw sections/keys via the INI API and feeds them
// here. Section naming:
//   "<group>"          → female rules for a built-in/custom group
//   "<group>.male"     → male rules
//   "Group:<name>"     → define/extend a custom group (female); "Group:<name>.male" for male
// Recognized keys (case-insensitive): "Mult.<Archetype>"=float, "Exclude"=comma list of archetype
// names (added to the hard-exclude set; a leading '-' on a name REMOVES a default exclude),
// "FrameBias"=float, and for custom groups "Match"=comma list of race substrings (applied to BOTH
// sexes). Unknown keys are ignored. Returns false if the section name is not recognized.
bool ApplyRulesSection(RaceRules& female, RaceRules& male,
                       const std::string& section,
                       const std::vector<std::pair<std::string, std::string>>& kv);

}  // namespace OBW
