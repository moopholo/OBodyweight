Scriptname OBW_Native Hidden

; Checks the acquired SKEE interface; logs a failure even with DebugLog disabled.
bool Function HasMorphInterface() global native
; Checks actor/base and SKEE before calling OBody's native preset methods.
bool Function CanApplyMorphs(Actor akActor) global native

; Returns the per-NPC "mock weight" (0-100) — drives the body-size morphs, never written
; to the actor's real weight.
float Function GetWeight(Actor akActor) global native

; Body mode 2 (Procedural Oriented) / 1 (OBody Presets, weight-driven): preset support.
; GetPresetSliders returns the slider names of an OBody preset (parsed from the BodySlide
; SliderPresets folder); GetPresetMorphs returns the matching morph values (aligned 1:1) for
; the given actor at its mock weight (faithful lerp where the author defined a weight curve,
; synthesized lean<->full on static volume sliders, never exceeding the preset's own values).
string[] Function GetPresetSliders(string asPreset) global native
float[] Function GetPresetMorphs(string asPreset, Actor akActor) global native

; Body mode 1 (preferred path): applies the OBody preset interpolated at the actor's mock weight
; ENTIRELY in C++ via SKEE — no 128-slider array cap, far fewer calls. asObKey = OBody's distribution
; key. Sets the morphs, drops OBody's "OBody"-key morphs, re-asserts "processed", and rebuilds.
; Returns false on failure and logs the reason; caller skips the application safely.
bool Function ApplyPresetMorphs(string asPreset, Actor akActor, string asObKey) global native
; Procedural modes: the WHOLE per-NPC morph suite in one call (set + blend + OBody clear/re-assert +
; clothed trim + one rebuild + neck color, all in a main-thread task). False = SKEE C++ iface missing.
bool Function ApplyAllMorphs(Actor akActor, bool abIsFemale, string asObKey, string asPreset) global native
; The OBody preset an NPC's body came from (modes 1/2); "" if pure procedural / not recorded.
string Function GetActorPresetName(Actor akActor) global native

; Debug: write a line into OBodyNGWeight.log (only when debug logging is ON).
Function Log(string asMsg) global native

; Debug logging master switch (MCM, off by default). Persisted in the cosave.
bool Function GetDebugLog() global native
Function SetDebugLog(bool abOn) global native

; Neck-seam COLOR fix: pull the actor's head facegen skin tint toward its body skin tone (reduces a head<->body
; tone mismatch at the neck). Strength 0..1 (0 = off), tuned in the MCM. NormalizeNeckColor runs the pass now.
Function NormalizeNeckColor(Actor akActor) global native
; OBody's apply method: re-morph the body + worn armor and DEFER the rebuild (no armor re-equip, no synchronous re-skin).
Function ApplyBody(Actor akActor) global native
float Function GetNeckColorFix() global native
Function SetNeckColorFix(float afStrength) global native

; True if the actor's source plugin is excluded (MCM checkbox or any OBodyNGWeight_Exclusions*.txt).
; OBW leaves such NPCs untouched (no procedural morphs, no preset re-application).
bool Function IsExcluded(Actor akActor) global native
; Exclude / re-include a SPECIFIC NPC by FormID (runtime toggle for a hotkey/MCM); persists to the MCM exclusion file.
Function SetActorExcluded(Actor akActor, bool abOn) global native
; Per-NPC exclusion hotkey (DirectInput scancode; 0 = unbound). Aim at an NPC + press to toggle its exclusion. MCM-bindable.
int Function GetExcludeKey() global native
Function SetExcludeKey(int aiKey) global native
; Export hotkey: aim at an actor (no one = yourself) + press -> writes the applied body as a BodySlide preset.
int Function GetExportKey() global native
Function SetExportKey(int aiKey) global native

; MCM exclusions. GetNpcPlugins = sorted list of plugins that ADD NPCs (for the checkbox list, capped
; at 128). IsPluginExcluded/SetPluginExcluded read/toggle the MCM-managed exclusion of one plugin
; (Set persists to OBodyNGWeight_Exclusions_MCM.txt, global across saves).
string[] Function GetNpcPlugins() global native
; Paged variants (the exclusion list is unbounded; the MCM shows ~120/page since Papyrus arrays cap at 128).
int Function GetNpcPluginCount() global native
string[] Function GetNpcPluginsPage(int aiPage, int aiPerPage) global native
bool Function IsPluginExcluded(string asPlugin) global native
Function SetPluginExcluded(string asPlugin, bool abOn) global native

; Distribution mode: 0=Random, 1=Seeded/Deterministic, 2=NpcDefault
int Function GetMode() global native
Function SetMode(int aiMode) global native

; Bias added to the generated weight (can be negative); the result is clamped to [0,100].
float Function GetBias() global native
Function SetBias(float afBias) global native

; Seed used in Seeded mode (per playthrough).
int Function GetSeed() global native
Function RegenerateSeed() global native

; Morph queue — C++ owns the queue; Papyrus only asks for the next actor.
Function QueueForMorphs(Actor akActor) global native
Actor Function GetNextMorphActor() global native
; Re-apply to EVERY loaded NPC (MCM button): re-queues them so the current generation + CBPC physics
; reapply without cell reloads. Keeps each body (same seed). Returns how many were queued.
int Function ReprocessAllLoaded() global native
; Procedural fallback (independent distribution): enqueue any loaded NPC OBody never handled, so procedural
; bodies apply even with an EMPTY preset library. Polled from OnUpdate. Returns how many were enqueued.
int Function SweepFallback() global native
; True if the queue still has actors waiting (used for throttled/lazy draining).
bool Function HasMorphsPending() global native

; Body shape mode: 0 = Procedural NiOverride morphs, 1 = OBody Presets (weight only),
; 2 = Procedural Oriented (procedural blended toward the OBody preset; strength = GetPresetOrient)
int Function GetBodyMode() global native
Function SetBodyMode(int aiMode) global native

; Global morph intensity multiplier (1.0 = default). Adjustable in the MCM.
float Function GetMorphScale() global native
Function SetMorphScale(float afScale) global native

; Preset orientation strength 0.0-1.0 for body mode 2 (0 = pure procedural, 1 = pure preset). MCM-tunable.
float Function GetPresetOrient() global native
Function SetPresetOrient(float afStrength) global native

; Fraction of "fantasy" (exaggerated) NPCs, 0.0-1.0. Adjustable in the MCM.
float Function GetFantasyRatio() global native
Function SetFantasyRatio(float afRatio) global native

; Fraction of NPCs with an "unusual body" (out of distribution: ultra-petite +
; disproportionate), 0.0-1.0. Adjustable in the MCM.
float Function GetUnusualRatio() global native
Function SetUnusualRatio(float afRatio) global native

; Fraction of NPCs with "unusual breasts" (extreme sag or extreme perk), 0.0-1.0.
float Function GetBreastUnusualRatio() global native
Function SetBreastUnusualRatio(float afRatio) global native

; Fraction of athletic FEMALES (visible muscle tone/definition), 0.0-1.0.
float Function GetAthleticRatio() global native
Function SetAthleticRatio(float afRatio) global native

; Race coherence strength, 0.0-1.0 (1.0 = full race-typed archetype distribution, 0.0 = race-blind uniform).
float Function GetRaceCoherence() global native
Function SetRaceCoherence(float afStrength) global native

; Natural-body ratio, 0.0-1.0: fraction of women given the BHUNP-derived "natural" body profile.
float Function GetNaturalRatio() global native
Function SetNaturalRatio(float afRatio) global native

; Curvy-body ratio, 0.0-1.0: fraction given the 3BA-style curvier profile (opposite pole of Natural). For BHUNP users.
float Function GetCurvyRatio() global native
Function SetCurvyRatio(float afRatio) global native

; Base-body preference: 0 = Auto-detect, 1 = CBBE (3BA), 2 = BHUNP. Selects the female BodyNet and MCM realism toggle.
int Function GetBaseBodyPref() global native
Function SetBaseBodyPref(int aiPref) global native
; Resolved base body (pref if set, else auto-detected): 0 = unknown/ambiguous, 1 = CBBE, 2 = BHUNP.
int Function GetBaseBody() global native

; Clothed refit, 0.0-0.5: OBW's own dressed-vs-nude body trim on the soft sliders (breasts/butt/belly).
float Function GetClothedRefit() global native
Function SetClothedRefit(float afRefit) global native
; Set/refresh the clothed-refit delta at the actor's current worn state. abRebuild=false only sets the delta
; (the caller's own ApplyBody rebuilds once); true rebuilds immediately (for standalone re-applies).
Function RefreshClothedRefit(Actor akActor, bool abRebuild) global native

; Re-roll key (DirectInput scancode). Default 26 = the [ / { key. Bindable in the MCM.
int Function GetReRollKey() global native
Function SetReRollKey(int aiKey) global native

; Master toggle for the female-body feature (morphs). Off = OBW ignores women (OBody/vanilla handle them).
bool Function GetFemaleBodies() global native
Function SetFemaleBodies(bool abOn) global native

; Master toggle for the male-body feature (weight + morphs). Off = OBW ignores men.
bool Function GetMaleBodies() global native
Function SetMaleBodies(bool abOn) global native

; Male build multiplier (1.0 = default). Scales the whole male body uniformly.
float Function GetMaleBuild() global native
Function SetMaleBuild(float afValue) global native

; Effective per-NPC intensity: realistic (~1.0) or fantasy (~1.3-2.2) x the global scale.
; Call ONCE per NPC and apply it to every slider.
float Function GetActorIntensity(Actor akActor) global native

; Procedural morph generation (no preset files needed).
; Call GetFrameScore ONCE per actor, then pass that T to each GetMorphValue call.
; This ensures all body parts are correlated (same frame score → proportional shape).
float Function GetFrameScore(Actor akActor) global native
float Function GetMorphValue(Actor akActor, float afFrameScore, string morphName) global native
; Volume sliders: GetMorphValue already multiplied by the per-NPC intensity and soft-capped
; to the sculpted vertex range (no spikes/breaks). Apply directly (do NOT multiply by kVol).
float Function GetVolumeMorph(Actor akActor, float afFrameScore, string morphName) global native

; Male morphs (HIMBO). Derived from build (muscle+fat) + archetype + traits + unusual.
float Function GetMaleMorphValue(Actor akActor, string morphName) global native
; Per-NPC male intensity (realistic/fantasy/unusual). For volume sliders.
float Function GetMaleIntensity(Actor akActor) global native
; Male VOLUME morph: GetMaleMorphValue * intensity, SOFT-CAPPED for HIMBO (no mesh break). Apply directly.
float Function GetMaleVolumeMorph(Actor akActor, string morphName) global native
; Male body archetype (HIMBO body types: Lean/Fit/Dadbod/Bodybuilder/Heavyset/Stocky...). Id 0-11 (-1 none).
int Function GetMaleArchetypeId(Actor akActor) global native
string Function GetMaleArchetypeName(Actor akActor) global native
; CBPC bounce(0)/collision(1) % for a male, from his archetype (firm Fit/Bodybuilder, soft Dadbod/Heavyset).
int Function GetMalePhysicsPercent(Actor akActor, int aiKind) global native

; Muscle tone 0-100 (same value that drives the MuscleAbs/Arms/Legs sliders).
int Function GetToneScore(Actor akActor) global native

; CBPC physics tier for this NPC's archetype: 0 = default, 1 = firm (toned/lean), 2 = soft
; (heavy/jiggly). Put the NPC in the matching faction so CBPC's IsInFaction picks the config.
int Function GetPhysicsTier(Actor akActor) global native
; Continuous physics % (0-100) that correlates with the real body (frame size + archetype softness).
; kind 0 = bounce, 1 = collision. ~32 = neutral. Used with ApplyBounce/CollisionInterpolation.
int Function GetPhysicsPercent(Actor akActor, int aiKind) global native

; Notable fine shapes (female): butt shape (heart/round/shelf/dimpled) + breast shape
; (round/teardrop/eastwest/wideset), or "" if none. Public body API for other mods.
string Function GetButtShapeName(Actor akActor) global native
string Function GetBreastShapeName(Actor akActor) global native

; Public body-type API: the NPC's full archetype (15 types, vs the 5 physics tiers).
; Id 0-14 (-1 none); name e.g. "Pear","BBW","Hourglass". For other mods.
int Function GetArchetypeId(Actor akActor) global native
string Function GetArchetypeName(Actor akActor) global native
; True if CBPC (cbp.dll) is loaded — gate the physics integration so non-CBPC users are fine.
bool Function HasCBPC() global native

; True only on the Skyrim VR runtime.
bool Function IsVR() global native
; VR-only: the actor the player is gazing at (cone-cast from the HMD), or None. Always
; None in desktop play, so the cone-cast never runs there.
Actor Function GetVRLookTarget() global native

; Regenerates weight and morphs for a specific actor (used by the re-roll hotkey).
; Removes it from the processed set, re-rolls the weight, and queues it for morphs.
Function RegenerateActor(Actor akActor) global native
Function MarkMorphsApplied(Actor akActor) global native
bool Function HasMorphsApplied(Actor akActor) global native

; ── BodyNet: the learned body generator (bodynet_*.obwnet in SKSE\Plugins\OBodyNGWeight) ────────────
; On = bodies come from the trained net instead of the procedural slider rules; the net's output still
; passes through the same per-slider ceilings, so it can never break a mesh. Determinism is unchanged
; (the noise is derived from the actor seed). Falls back to the procedural path per sex if that sex has
; no model installed.
bool Function GetNeuralBody() global native
Function SetNeuralBody(bool abOn) global native
; True if at least one model file loaded - the MCM greys the toggle out when none is installed.
bool Function GetNeuralAvailable() global native
