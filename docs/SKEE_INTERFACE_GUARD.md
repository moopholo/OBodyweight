# SKEE interface guard — 2026-09-20

OBW checks the exchange dispatch, returned interface map, BodyMorph pointer and a nonzero
reported interface version at PostPostLoad. Failure logs its specific cause at error level
even with DebugLog off. The plugin stays loaded; morph application is skipped while the
required interface is unavailable. Restart after correcting the RaceMenu/SKEE installation.

Both procedural and simulated-weight entry points check availability before touching the
actor's preset. Native application also checks the task interface and actor, and reports
empty preset data or computation exceptions. Queued tasks recheck interface and actor.

The legacy Papyrus application fallbacks were removed: they invoked
OBodyNative.ResetActorOBodyMorphs, whose ClearActorMorphs implementation dereferences its
own SKEE pointer without checking it. An OBW interface check cannot establish whether
OBody's separately acquired pointer is valid, so OBW no longer invokes that reset.

This guards missing/invalid initialization results. It does not catch access violations
inside a damaged third-party DLL, nor prove the cause of the expired reported crash log.
Version nonzero is a basic sanity check, not proof of ABI compatibility with every version.

## Follow-up endpoint audit

- Added CanApplyMorphs to check actor, ActorBase and SKEE before OBW's two Papyrus
  application paths query OBody's preset registry. OBody's IsFemale dereferences ActorBase.
  Native application entry points repeat the check for direct callers.
- Queued full-body and simulated-weight applications check ActorBase and loaded 3D again
  when the task runs. Missing data produces a warning and skips the queued application.
  This is defensive lifecycle handling, not a demonstrated crash reproduction.
- Hotkey and export messages now tolerate a null display-name result.
- Race, VR camera/root, process lists, configuration form/file lookups, player cleanup,
  face node/material and existing SKEE use were inspected for null guards. BSFixedString
  c_str() already returns an empty string for missing storage in the vendored CommonLib.
- DLL and all three Papyrus scripts compiled successfully. In-game unloaded-actor and
  missing-interface fault scenarios remain untested; a non-null pointer cannot prove lifetime
  or thread safety. OBody NG itself still contains unchecked public entry points; its DLL
  was not modified by this OBW audit.
