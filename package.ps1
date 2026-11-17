# Builds the Nexus upload archive from the LIVE mod folder.
#
# Why a script and not a manual zip: the mod folder IS the game's Data dir (MO2), so it also holds test
# leftovers, the plugin sources, the tools and the preset bank - none of which belong in a release. This
# copies an explicit allow-list into dist\pkg and zips it, so a release can never accidentally ship a
# scratch file, and re-running it after a rebuild always produces the same layout.
#
# FOMOD installs whole folders (<folder source="Core">), so adding a file under Core needs no XML change.
#
#   .\package.ps1              -> version read from plugin\CMakeLists.txt
#   .\package.ps1 -Version x.y.z

param([string]$Version = "")

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot
$pkg  = Join-Path $root "dist\pkg"
$core = Join-Path $pkg "Core"
$rootFull = [IO.Path]::GetFullPath($root).TrimEnd('\')
$pkgFull = [IO.Path]::GetFullPath($pkg)
if (-not $pkgFull.StartsWith($rootFull + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw "package staging escaped the mod folder: $pkgFull"
}

if (-not $Version) {
    $m = Select-String -Path (Join-Path $root "plugin\CMakeLists.txt") -Pattern 'project\(OBodyNGWeight VERSION ([\d.]+)'
    if (-not $m) { throw "could not read the version from plugin\CMakeLists.txt" }
    $Version = $m.Matches[0].Groups[1].Value
}
Write-Host "packaging OBodyNG Weight $Version" -ForegroundColor Cyan

# ── the release allow-list: source (relative to the mod root) -> destination inside Core ──
$files = @(
    @{ src = "OBodyNGWeight.esp";                                  dst = "" },
    @{ src = "LICENSE";                                            dst = "" },
    @{ src = "CHANGELOG.txt";                                      dst = "" },
    @{ src = "SKSE\Plugins\OBodyNGWeight.dll";                     dst = "SKSE\Plugins" },
    @{ src = "SKSE\Plugins\OBodyNGWeight.ini";                     dst = "SKSE\Plugins" },
    @{ src = "SKSE\Plugins\OBodyNGWeight_Exclusions.txt";          dst = "SKSE\Plugins" },
    # Fork: data-driven archetype + race/group rule tables (read at startup; built-in defaults if absent).
    @{ src = "SKSE\Plugins\OBodyNGWeight_Archetypes.ini";          dst = "SKSE\Plugins" },
    @{ src = "SKSE\Plugins\OBodyNGWeight_MaleArchetypes.ini";      dst = "SKSE\Plugins" },
    @{ src = "SKSE\Plugins\OBodyNGWeight_Races.ini";               dst = "SKSE\Plugins" },
    @{ src = "SKSE\Plugins\CBPCBounceinterpolationconfig_OBW.txt"; dst = "SKSE\Plugins" },
    @{ src = "SKSE\Plugins\CBPCCollisioninterpolationconfig_OBW.txt"; dst = "SKSE\Plugins" },
    @{ src = "SKSE\Plugins\CBPConfig_zzOBW_BodyFirm.txt";          dst = "SKSE\Plugins" },
    @{ src = "SKSE\Plugins\CBPConfig_zzOBW_BreastFirm.txt";        dst = "SKSE\Plugins" },
    # The trained generators. Ship in Core, not as an option: without them the AI toggle is greyed out,
    # so an install without the models is just the old mod with a dead switch.
    @{ src = "SKSE\Plugins\OBodyNGWeight\bodynet_f3ba.obwnet";     dst = "SKSE\Plugins\OBodyNGWeight" },
    @{ src = "SKSE\Plugins\OBodyNGWeight\bodynet_fbhunp.obwnet";   dst = "SKSE\Plugins\OBodyNGWeight" },
    @{ src = "SKSE\Plugins\OBodyNGWeight\bodynet_mhimbo.obwnet";   dst = "SKSE\Plugins\OBodyNGWeight" },
    @{ src = "Scripts\OBW_MCM.pex";                                dst = "Scripts" },
    @{ src = "Scripts\OBW_Native.pex";                             dst = "Scripts" },
    @{ src = "Scripts\OBW_Quest.pex";                              dst = "Scripts" },
    # Papyrus sources ship too: other authors extend the MCM, and a .pex with no source is hostile.
    @{ src = "Scripts\Source\User\OBW_MCM.psc";                    dst = "Scripts\Source\User" },
    @{ src = "Scripts\Source\User\OBW_Native.psc";                 dst = "Scripts\Source\User" },
    @{ src = "Scripts\Source\User\OBW_Quest.psc";                  dst = "Scripts\Source\User" }
)

if (Test-Path $pkgFull) { Remove-Item -LiteralPath $pkgFull -Recurse -Force }
foreach ($f in $files) {
    $s = Join-Path $root $f.src
    if (-not (Test-Path $s)) { throw "MISSING: $($f.src) - build first (build.ps1 / cmake) before packaging" }
    $d = if ($f.dst) { Join-Path $core $f.dst } else { $core }
    New-Item -ItemType Directory -Force -Path $d | Out-Null
    Copy-Item $s $d -Force
}

# ── release defaults: never ship the developer's test settings ───────────────────────────────────
# The INI is copied from the LIVE mod folder, which is also where features get toggled on to test them.
# The 1.6.0 build was packaged with NeuralBody=1 still set from an in-game test run, which would have
# shipped the new generator ON by default while the changelog told users it was off. Any key a developer
# is likely to flip while testing gets forced back to its release value here, and the change is printed
# so it is never silent.
$releaseDefaults = @{ "NeuralBody" = "0"; "DebugLog" = "0" }
$iniPath = Join-Path $core "SKSE\Plugins\OBodyNGWeight.ini"
$ini = Get-Content $iniPath -Raw
foreach ($k in $releaseDefaults.Keys) {
    $want = $releaseDefaults[$k]
    if ($ini -match "(?m)^$k=(.*)$" -and $Matches[1].Trim() -ne $want) {
        Write-Host "  reset $k=$($Matches[1].Trim()) -> $want (test setting, not a release default)" -ForegroundColor Yellow
        $ini = $ini -replace "(?m)^$k=.*$", "$k=$want"
    }
}
# Written as bytes with CRLF: the Win32 profile API that reads this file needs CRLF, and a plain
# Set-Content here has silently converted the whole INI to LF before.
[IO.File]::WriteAllText($iniPath, ($ini -replace "`r`n", "`n" -replace "`n", "`r`n"), (New-Object Text.UTF8Encoding $false))

# Every FOMOD option must point to a real folder in the archive. Build the three
# starting INIs from the release INI so future defaults stay in sync.
foreach ($preset in @(
    @{ name = "Balanced";  scale = "1.0";  fantasy = "0.15" },
    @{ name = "Realistic"; scale = "0.95"; fantasy = "0.0" },
    @{ name = "Fantasy";   scale = "1.2";  fantasy = "0.4" }
)) {
    $dir = Join-Path $pkg "Presets\$($preset.name)\SKSE\Plugins"
    New-Item -ItemType Directory -Force -Path $dir | Out-Null
    $presetIni = $ini -replace '(?m)^MorphScale=.*$', "MorphScale=$($preset.scale)"
    $presetIni = $presetIni -replace '(?m)^FantasyRatio=.*$', "FantasyRatio=$($preset.fantasy)"
    [IO.File]::WriteAllText((Join-Path $dir "OBodyNGWeight.ini"),
        ($presetIni -replace "`r`n", "`n" -replace "`n", "`r`n"), (New-Object Text.UTF8Encoding $false))
}

# Staleness guard: a .pex older than its .psc means the scripts were edited and never recompiled - the
# exact failure that shipped an MCM option with no Papyrus behind it once already.
foreach ($n in "OBW_MCM", "OBW_Native", "OBW_Quest") {
    $pex = Get-Item (Join-Path $root "Scripts\$n.pex")
    $psc = Get-Item (Join-Path $root "Scripts\Source\User\$n.psc")
    if ($pex.LastWriteTime -lt $psc.LastWriteTime) { throw "$n.pex is OLDER than $n.psc - run build.ps1" }
}
$dll = Get-Item (Join-Path $root "SKSE\Plugins\OBodyNGWeight.dll")
Write-Host ("  dll {0:yyyy-MM-dd HH:mm}  {1:N0} KB" -f $dll.LastWriteTime, ($dll.Length / 1KB))

# FOMOD version must match, or Vortex/MO2 shows the previous release's number.
$infoPath = Join-Path $pkg "fomod\info.xml"
New-Item -ItemType Directory -Force -Path (Split-Path $infoPath) | Out-Null
Copy-Item (Join-Path $root "fomod\info.xml") $infoPath -Force
Copy-Item (Join-Path $root "fomod\ModuleConfig.xml") (Join-Path $pkg "fomod") -Force
(Get-Content $infoPath -Raw) -replace '<Version>[^<]*</Version>', "<Version>$Version</Version>" |
    Set-Content $infoPath -Encoding UTF8

$zip = Join-Path $root "dist\OBodyNG Weight $Version.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $pkg "*") -DestinationPath $zip -CompressionLevel Optimal

$z = Get-Item $zip
Write-Host ("`n{0}  ({1:N2} MB, {2} files)" -f $z.Name, ($z.Length / 1MB),
    (Get-ChildItem $pkg -Recurse -File).Count) -ForegroundColor Green
