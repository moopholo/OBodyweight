# OBodyNG Weight

Procedural NPC weight **and** body randomization for **OBody NG** — for both **women
(CBBE 3BA)** and **men (HIMBO)**. Every NPC gets a unique, proportional body with no
BodySlide preset library required, and equipped clothing follows the generated shape.

## Features

- **Procedural body shapes** generated per NPC via SKEE morphs — no preset files needed.
  Women use CBBE 3BA sliders, men use HIMBO. Magnitudes calibrated to real BodySlide presets.
- **Three body modes** — *Procedural* (every body generated from scratch, no presets needed);
  *OBody Sim Weight* (keeps OBody's preset shape but re-applies each **auto-distributed** preset at
  that NPC's simulated body weight, so NPCs sharing a preset differ in size/fullness instead of being
  clones — and presets with no weight range get a synthesized lean↔full range; presets you assign
  **manually** via OBody's menu are left untouched so OBody's own features keep working);
  *Procedural Oriented* (start from a generated body, then blend it part-way toward that
  NPC's OBody preset — a slider sets how far, 0% pure generated to 100% the preset itself).
- **Body archetypes (women)** — each NPC gets one of 15 coherent body types: Balanced, Slim,
  Rectangle, Pear, Top-heavy, Hourglass, Voluptuous, Apple/Soft, BBW, Athletic, Athletic-curvy,
  Obese, Stocky, Petite, Amazon. The archetype drives bust/waist/hips/belly/tone together, so
  bodies read as intentional, recognizable shapes — not random feature combinations.
- **Average-centered distribution** — the median body is balanced/average in every measure;
  petite and curvy bodies taper off to the sides. Extremes are the minority, not the norm.
- **Seam-free** — the NPC's real weight value is never changed; body-size variety comes
  purely from morphs, so head, body and worn outfit always match (no neck seams).
- **Realistic + fantasy mix** — most NPCs are grounded; a tunable minority are exaggerated.
- **Light traits** — small per-NPC variations layered on top of the archetype (perky/saggy
  derived from bust size, etc.). Men: big/flat pecs, V-taper, barrel chest, gut, thick/skinny legs.
- **Per-body physics (CBPC, optional)** — physics scales with the actual body: bounce amplitude and
  collider size follow size + softness (bigger/softer = more jiggle and larger colliders; toned/lean
  = firmer and tighter). Soft dependency on CBPC — without it, nothing changes. Drop-in config files
  are included.
- **Muscle tone** — women have a tunable "athletic" fraction with visible abs/arm/leg
  definition (and a rare **snu snu** super-toned Amazon); men get tone automatically from
  their build. Suppressed by body fat, so it reads correctly.
- **Unusual bodies** — a rare out-of-distribution roll: ultra-petite/ultra-thick (women) or
  ultra-skinny/ultra-huge (men), disproportionate and atypical.
- **Re-roll hotkey** (re-bindable, default `[` / `{`) — aim at an NPC for a brand-new body.
  **VR-ready**: in VR it targets the NPC in your HMD gaze, or yourself if nothing is in view.
- **Per-sex toggles** — turn the female and male body features on/off independently (let OBody/
  vanilla handle a sex), plus a **Male build** slider to scale male corpulence; all parts scale
  together so proportions hold.
- **Clothing refit** — armor built with morphs follows the new shape.
- **Performance** — distance-aware lazy loading drains the morph queue gradually and
  nearest-first, and bodies rebuild with the fewest passes (lighter on RaceMenu overlays).
- **ESL-flagged** — doesn't use a load-order slot. Works on Skyrim SE, AE and VR.
- All logic, RNG and state live in the C++ SKSE plugin; Papyrus is a thin relay.

## MCM

Generation mode (Procedural Morphs / OBody Sim Weight / Procedural Oriented), Weight mode
(Random / Seeded), **Bias** (global heavier/leaner), **Female bodies** /
**Male bodies** toggles, **Male build**, **Morph intensity**, **Fantasy NPCs %**,
**Unusual bodies %**, **Unusual breasts %**, **Athletic women %**, **Preset orientation**
(Oriented mode), Seed, a re-bindable **Re-roll key**, and a **Debug logging** toggle. Options
grey out when they don't apply to the selected mode.

## Requirements

- Skyrim SE/AE + **SKSE64**
- **Address Library for SKSE Plugins**
- **OBody NG** (hard requirement — master of the plugin)
- **RaceMenu / SKEE** (NiOverride body morphs)
- **SkyUI** (MCM)
- A **CBBE / CBBE 3BA** female body and a **HIMBO** male body, both **built in BodySlide with
  "Build Morphs" checked** (the `.tri` data). Build your armors with morphs for clothing to follow.
- *(Optional)* **CBPC** — enables the per-body physics. Without it, OBW just skips that step.

## Installation

Install with a mod manager and enable `OBodyNGWeight.esp` (it's ESL-flagged, so it doesn't use
a regular load-order slot). Load it after OBody NG. Open the MCM, choose Procedural Morphs, and
(recommended) set OBody's actor-selection hotkey to None so the re-roll key (default `[` / `{`,
re-bindable in the MCM) only triggers this mod.

## How it works

OBody (which has both female and male preset databases) fires its actor-generated event; this
mod applies its own body morphs under key `OBW`, branching on sex. In **Procedural** mode it
generates the shape directly. In **OBody Sim Weight** mode it re-applies OBody's **auto-distributed**
preset at a per-NPC simulated body weight (so NPCs sharing a preset vary, and weightless presets get a
synthesized lean↔full range) — presets you assign **manually** through OBody's menu are deliberately
left untouched so OBody's own on-demand features keep working. **Procedural Oriented** blends the
generated shape toward that preset.

## Tuning body shapes — `OBodyNGWeight_Archetypes.ini`

Every generated body is one of 24 **archetypes** (Slim Thick, Pear, Hourglass, BBW, Athletic…),
each a set of shape offsets plus a **weight** (how often it's rolled). You can retune both — the
*weights* (which shapes dominate the world) and the *shapes themselves* — by editing
`Data/SKSE/Plugins/OBodyNGWeight_Archetypes.ini`, **no rebuild required**. Edit, save, restart.

- One `[Section]` per archetype. Every key is optional — **omit it to keep the built-in value**;
  delete a whole section to leave that archetype untouched.
- Values are **0–100 slider space**. `Waist` > 0 = **more cinch** (narrower), < 0 = wider.
  `Weight` is the relative roll frequency (`0` = never).

```ini
[Slim Thick]
Weight=12.0     ; rolled often
Butt=26         ; +26 in slider space
Hips=18
Thighs=20
Waist=35        ; strong cinch
Breasts=6
Tone=25         ; some muscle definition
Intensity=0.02
```

Keys per section: `Weight, FrameBias, FrameScale, Breasts, Butt, Hips, Thighs, Belly, Waist,
Arms, Shoulders, Tone, Intensity`. The shipped file is pre-tuned toward a curvy / slim-thick
family; a missing file simply falls back to the built-in defaults.

## Compatibility

- Female slider names target CBBE 3BA; male names target HIMBO. Other bodies use different
  slider names and would need the tables adjusted.
- **Exclude NPCs by mod** — to keep OBW off a follower/NPC mod's hand-made bodies, open the MCM's
  **Exclusions** page and tick the mod: it lists every plugin that adds NPCs, one checkbox each.
  Excluded NPCs are left to OBody / vanilla / the source mod. Choices are saved globally (across
  saves) and apply to NPCs generated afterward (bodies already applied aren't reverted).
  Advanced: you can also list plugin names (one per line, `.esp` / `.esl` / `.esm`) in any
  `SKSE\Plugins\OBodyNGWeight_Exclusions*.txt` file — merged on game start, so patches can ship
  their own. Case-insensitive.

## FAQ

**"NPCs still get an OBody preset assigned — no matter what I do. Is it broken? Does it assign RaceMenu
presets?"** No, and no:

- OBW changes the **body** (via OBody). It **never touches RaceMenu / High Poly Head *face* presets** —
  those are a separate face system. So any face preset you see is unrelated to OBW.
- In **OBody Sim Weight** mode OBW *keeps* the OBody preset on purpose — it re-applies that preset at a
  per-NPC simulated weight. That is exactly what gives variety to presets that look identical at weight 0
  and 100. So a preset *staying assigned* is correct in this mode.
- For a body generated **from scratch with no preset**, choose **Procedural Morphs** mode. Even then:
  OBody's own menu may still *display* a preset name (stale — the body underneath is OBW's), presets you
  assigned **manually** through OBody's menu are deliberately left alone, and distant / just-spawned NPCs
  are processed lazily (nearest-first). Undress a nearby NPC to see OBW's actual generated body.

## For mod authors

OBW exposes the generated body type so other plugins can react to it (Papyrus, soft dependency):

```
int    OBW_Native.GetArchetypeId(Actor akActor)    ; 0-14, or -1 if none
string OBW_Native.GetArchetypeName(Actor akActor)  ; "Pear", "BBW", "Hourglass", ...
```

Deterministic per NPC + playthrough seed (matches the body they were given). Gate your calls so
they only run when OBW is installed.

## Credits

- **OBody NG** — Aietos (original **OBody** by Sinhime)
- **RaceMenu / SKEE** — Expired · **SKSE64** — ianpatt, behippo, purplelunchbox
- **Address Library** — meh321 · **CommonLibSSE-NG** — CharmedBaryon (orig. Ryan-rsm-McKenzie)
- **SkyUI** — SkyUI Team · **CBBE / 3BA** — Ousnius & Caliente, Acro748 · **HIMBO** — Shino
- **BodySlide** — Ousnius & Caliente · **spdlog** — gabime
