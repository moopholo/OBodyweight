# Save-time growth — investigation (OPEN, not diagnosed)

**Report (Nexus, 2026-08-10, NeyriXX):** ~8 second save times after ~7h of gameplay. "Could this, or the
original OBody, be the culprit?" MisterBoxer replied that meeting many NPCs racks up a large morph
database; NeyriXX confirmed **S.L.A.C.K. does not dent it**, and that **clearing that data with a save
cleaner fixes it** (at the cost of losing the NPC morphs).

**Status: investigating. We do NOT know it is OBW.** OBody NG writes body morphs too, and the reporter
themselves raised OBody as a candidate. Everything below is labelled measured vs hypothesis.

## MEASURED — OBW's own cosave is not the cause

Read straight from `WeightManager::Save`:
- 20 records total; 19 are scalar settings (a few bytes each).
- Exactly ONE per-actor collection: `_overrideSeed`, written as `count` + `(FormID, seed)` pairs =
  **8 bytes per NPC**. At 10,000 NPCs that is 80 KB.
- **Zero** form lookups, sorting or allocation-heavy work in `Save()` — it is a flat walk of a small map.

So OBW's serialization cannot account for seconds. **This also settles the "save incrementally / only
write the tracked list when it changed" idea: there is nothing measurable to save.** A dirty flag there
would be motion without effect, and would add a correctness risk (a cosave record that is skipped is
*absent* on load, not "unchanged" — SKSE has no incremental record API).

## HYPOTHESIS — the SKEE body-morph database, and whether OBW enlarges it

The thing a save cleaner wipes is SKEE's per-actor body-morph store (that is also why S.L.A.C.K., which
speeds up cosave *writing*, does not help — the cost is the data volume, not the encoder).

Both mods feed that store. The question is whether OBW feeds it **more** than OBody alone would:

- OBW writes ~52 morph entries per female NPC (49 sliders + 3 BHUNP aliases) under key `OBW`, and
  clears OBody's key. So per NPC it roughly SUBSTITUTES OBody's entries rather than adding to them.
- **But OBW reaches more NPCs.** In the procedural body modes (0 and 2 — the default), `ActorLoadSink`
  watches **every humanoid NPC that loads** (`ActorTypeNPC`, non-excluded) and the fallback sweep
  morphs it even if OBody never handled it. That is a deliberate feature (v1.4.2: "procedural works
  with an empty preset library / for OBody-skipped NPCs"), and it means the population OBW morphs is a
  superset of OBody's.

If the store's cost scales with *NPCs ever morphed*, a superset of NPCs means a bigger store. That is a
plausible mechanism — **it is not proof**, and the size difference has not been measured.

## What would actually settle it

Ask the reporter for:
1. The size of their `.skse` cosave next to the `.ess` (that is where the morph store lives).
2. Their OBW **body mode** — the fallback only runs in Procedural / Procedural Oriented, NOT in
   "OBody Sim Weight" (mode 1). If save time is normal in mode 1 and bad in mode 0/2, that points here.
3. Whether **male bodies** are enabled (doubles the morphed population).
4. An A/B on the same save: OBW disabled (OBody alone) vs enabled — cosave size and save time.

## Candidate fix, IF the hypothesis is confirmed

Do not persist what we can regenerate. OBW's architecture already **re-applies morphs on every 3D load**
(that is the whole OBody re-assert design), and a body is **deterministic from the actor seed**
(`GetActorSeed` = the persisted override seed, else `formID ^ _seed`). So the stored morphs are largely
redundant: clearing OBW's morph keys when an actor's 3D unloads would bound the store to
currently-loaded NPCs, and the same NPC would come back with **exactly the same body** on their next
load.

Risks to weigh before doing this to a public mod:
- Any consumer reading OBW's morphs while the actor is unloaded would see nothing.
- It trades save size for a little extra work on each load (already paid today by the re-assert).
- It changes behaviour for every user, so it needs an INI/MCM gate and in-game validation first.

## Log

- 2026-08-30 — code audit done (the MEASURED section). No fix implemented: the requested incremental
  save was measured as pointless, and the real candidate is still an unconfirmed hypothesis. Next step
  is data from the reporter, not code.
