#pragma once

#include <string>
#include <unordered_set>
#include <vector>

namespace RE { class Actor; }

namespace OBW::Config {

// Default values read from Data/SKSE/Plugins/OBodyNGWeight.ini at plugin load.
// These seed a new game (and Revert); existing saves restore from the cosave instead.
// A FOMOD installs one of several INI variants (Realistic / Balanced / Fantasy).
inline float g_defaultMorphScale   = 1.0f;
inline float g_defaultPresetOrient = 0.5f;   // body mode 2 blend strength (0 = pure procedural, 1 = pure preset)
inline float g_defaultFantasyRatio = 0.15f;
inline float g_defaultUnusualRatio       = 0.06f;
inline float g_defaultBreastUnusualRatio = 0.06f;
inline float g_defaultAthleticRatio      = 0.15f;
// Race coherence strength (0 = off/uniform archetype distribution, 1 = full race-typed). Biases which
// body archetypes each race trends toward (Orc big/broad, Bosmer petite, Altmer slim, Elder soft...).
inline float g_defaultRaceCoherence      = 1.0f;
// Natural-body ratio: fraction of women given the BHUNP-derived "natural" body profile (moderate/wider/
// lower/closer bust with drape, less-cinched wider waist, softer belly). 0.0-1.0. A natural<->curvy axis.
inline float g_defaultNaturalRatio       = 0.20f;
// Curvy-body ratio: fraction of women given the 3BA-style curvier profile (the opposite pole of Natural). For
// BHUNP users who want some exaggerated bodies. 0.0-1.0. Off by default (CBBE users already have curvy default).
inline float g_defaultCurvyRatio         = 0.0f;
// Base body the setup renders: 0 = Auto-detect (from the load order), 1 = CBBE (3BA), 2 = BHUNP. Selects
// the female BodyNet and gates which realism toggle the MCM surfaces (Natural for CBBE, Curvy for BHUNP).
inline int   g_defaultBaseBody           = 0;
// Clothed refit: OBW's own dressed-vs-nude body trim on the soft sliders (breasts/butt/belly), 0.0-0.5.
// The desirable half of OBody's ORefit, owned by OBW. 0 = off (dressed body == nude body).
inline float g_defaultClothedRefit       = 0.10f;
// BodyNet (learned body generator): OFF by default. A new feature on a public mod earns its default
// only after in-game validation - until then the procedural path everyone already has stays in charge.
inline bool  g_defaultNeuralBody         = false;
inline int   g_defaultReRollKey          = 26;  // [ / { key
// Per-NPC exclusion hotkey (DirectInput scancode; 0 = unbound). Aim at an NPC + press to toggle its OBW
// exclusion. Loaded from the INI at launch; MCM-bindable (SetExcludeKey rewrites the INI so it persists).
inline int   g_excludeKey                = 0;
// Body-preset EXPORT hotkey (DirectInput scancode; 0 = unbound). Aim at an actor (or at no one = yourself)
// + press to write their applied OBW body as a BodySlide SliderPresets .xml. INI-persisted, MCM-bindable.
inline int   g_exportKey                 = 0;
// Female bodies: when false, OBW leaves female NPCs entirely alone (no morphs) — OBody /
// vanilla handle them. Toggleable in the MCM.
inline bool  g_defaultFemaleBodies       = true;
// Male bodies: when false, OBW leaves male NPCs entirely alone (no weight, no morphs) —
// OBody / vanilla handle them. Toggleable in the MCM.
inline bool  g_defaultMaleBodies         = true;
// Male build multiplier (1.0 = default). Scales the whole male body uniformly.
inline float g_defaultMaleBuild          = 1.0f;
// Debug logging (verbose per-NPC/per-event diagnostics). Off by default; MCM-toggleable.
inline bool  g_defaultDebugLog           = false;
// Neck-seam COLOR fix: blend the head's facegen skin TINT toward the body skin tone (bodyTintColor) by this
// strength (0 = off, 1 = head tint forced to the body tone). Reduces a head<->body tone mismatch at the neck
// (a runtime tint pull, not the baked texture - so it lessens, not always erases, the seam). MCM-tunable.
inline float g_defaultNeckColorFix       = 0.5f;

// Parse the INI. Call once in SKSEPluginLoad, before WeightManager is constructed.
void Load();

// Plugins (lowercased) whose NPCs OBW leaves untouched. Two effective sources:
//  - g_mcmExcluded : managed by the MCM checkboxes, saved to OBodyNGWeight_Exclusions_MCM.txt (global).
//  - g_fileExcluded: any other OBodyNGWeight_Exclusions*.txt (hand-edited / patches), read-only.
inline std::unordered_set<std::string> g_mcmExcluded;
inline std::unordered_set<std::string> g_fileExcluded;
// FormID-level exclusions (exclude ONE specific NPC, not a whole plugin), keyed "pluginlower|0xlocalid".
// Parallels the name sets: MCM/runtime-managed (saved to the MCM file) + read-only from hand-edited files.
inline std::unordered_set<std::string> g_mcmExcludedForms;
inline std::unordered_set<std::string> g_fileExcludedForms;

// (Re)load both sets from disk. Call once at load.
void LoadExclusions();

// True if the actor's base (or reference) originates from / is overridden by an excluded plugin.
bool IsActorExcluded(RE::Actor* a_actor);

// MCM helpers: checkbox state, toggle (rewrites the MCM file), and the list of NPC-adding plugins.
bool IsPluginExcluded(const char* a_plugin);
void SetPluginExcluded(const char* a_plugin, bool a_on);
// Exclude / re-include a SPECIFIC actor by its base FormID (runtime toggle for a hotkey/MCM); persists to the
// MCM file. Returns false when the NPC has no durable identity to key (fully runtime-spawned actor: dynamic
// base AND dynamic reference) — callers must NOT report success then.
bool SetActorExcluded(RE::Actor* a_actor, bool a_on);
// Set the per-NPC exclusion hotkey (updates the runtime value + rewrites the INI so it persists).
void SetExcludeKey(int a_key);
void SetExportKey(int a_key);
std::vector<std::string> GetNpcPlugins();

}  // namespace OBW::Config
