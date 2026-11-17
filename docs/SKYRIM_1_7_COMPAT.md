# Skyrim 1.7 / Address Library compatibility

## Diagnosis (2026-11-04)

The reported `REL/ID.h(219): failed to open address library file` comes from the CommonLibSSE-NG build bundled into OBW 1.6.1. Its `ID.h` constructs `Data/SKSE/Plugins/versionlib-<game-version>.bin` for AE and fails before the OBW plugin can log anything if the file is absent from the game's effective Data directory. For Steam Skyrim 1.7.104.0, that path is `Data/SKSE/Plugins/versionlib-1-7-104-0.bin`.

The compiled CommonLib dependency was the CharmedBaryon fork at commit `b93280e` (2024-09-03). It also hardcodes AE Address Library format 2. Skyrim 1.7.99+ Address Library uses format 5. Therefore installing the missing file alone would uncover a second incompatibility in the old DLL.

The project now pins `alandtse/CommonLibSSE-NG` v15.0.0, which reads format 5 and includes 1.7 runtime layouts while retaining SE/AE/VR multi-runtime support. OBW 1.6.2 builds against it. The migration required adapting geometry shader access, crosshair targeting and HUD notifications to the current API.

## User installation check

For **Steam Skyrim 1.7.104.0** install the matching **SKSE 2.3.1** and **Address Library All in One (1.7.104.0) v13**. In MO2, confirm that `Data/SKSE/Plugins/versionlib-1-7-104-0.bin` is visible in the effective Data tree. The standalone "Offsets 1 7 104 0" file on the Nexus page is for authors; users should install **All in One**. Other Skyrim executables require their own matching SKSE and Address Library versions.

Do not rename a library file from another game version: address offsets are runtime-specific. If the file is present and an old OBW DLL now reports "Unsupported address library format: 5", replace the DLL with the updated build. If the file is present and the updated DLL still fails, collect the exact game executable version, SKSE version, MO2 effective Data path and SKSE log.

## Validation gates

- Clean compile of the pinned CommonLib and OBW for SE, AE and VR: passed (VS 2022 x64 Release).
- Format-5 `load_v5` and `versionlib-{}.bin` lookup inspected in pinned source.
- Runtime loading on 1.6.1170 and 1.7.104 still needs an in-game check. The local Skyrim executable is 1.6.1170; 1.7 gameplay validation needs a separate installation or user report.
- Release package `dist/OBodyNG Weight 1.6.2.zip` contains the rebuilt DLL, updated version and dependency note. The previous live DLL is backed up as `dist/OBodyNGWeight-1.6.1-pre-1.7.dll`.

Sources: [SKSE releases](https://skse.silverlock.org/), [Address Library files](https://www.nexusmods.com/skyrimspecialedition/mods/32444?tab=files), [maintained CommonLib changelog](https://github.com/alandtse/CommonLibSSE-NG/blob/ng/CHANGELOG.md).
