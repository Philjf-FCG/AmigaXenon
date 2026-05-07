#Requires -Version 7.0
<#
.SYNOPSIS
    Package RetroScreen into a distributable build.

.DESCRIPTION
    Runs UAT BuildCookRun to produce a staged build, then scaffolds an empty
    EmulatorData directory tree (with instructional READMEs), copies README.md
    and LEGAL.md, and verifies no copyrighted assets are bundled.

.PARAMETER OutputDir
    Root directory for the packaged build. Defaults to Build/Package relative
    to the project root.

.PARAMETER UE5Root
    Path to your Unreal Engine 5 installation. Defaults to the value of the
    UE5_ROOT environment variable, then falls back to the most common install
    path.

.PARAMETER Configuration
    Build configuration: Development (default) or Shipping.

.EXAMPLE
    .\scripts\package.ps1
    .\scripts\package.ps1 -OutputDir D:\Releases\RetroScreen-1.0 -Configuration Shipping
#>

param(
    [string] $OutputDir     = "",
    [string] $UE5Root       = "",
    [string] $Configuration = "Development"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# ---------------------------------------------------------------------------
# Resolve paths
# ---------------------------------------------------------------------------

$ProjectRoot = Split-Path -Parent $PSScriptRoot
$UProjectFile = Join-Path $ProjectRoot "RetroScreen.uproject"

if (-not (Test-Path $UProjectFile)) {
    Write-Error "Cannot find RetroScreen.uproject at '$UProjectFile'. Run this script from inside the project tree."
}

if ($OutputDir -eq "") {
    $OutputDir = Join-Path $ProjectRoot "Build\Package"
}

if ($UE5Root -eq "") {
    $UE5Root = $env:UE5_ROOT
}

if ($UE5Root -eq "" -or $null -eq $UE5Root) {
    # Common Epic Games Launcher install paths
    $candidates = @(
        "C:\Program Files\Epic Games\UE_5.7",
        "C:\Program Files\Epic Games\UE_5.6",
        "C:\Program Files\Epic Games\UE_5.5"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { $UE5Root = $c; break }
    }
}

if ($UE5Root -eq "" -or $null -eq $UE5Root -or -not (Test-Path $UE5Root)) {
    Write-Error "Unreal Engine root not found. Set UE5_ROOT or pass -UE5Root."
}

$RunUAT = Join-Path $UE5Root "Engine\Build\BatchFiles\RunUAT.bat"
if (-not (Test-Path $RunUAT)) {
    Write-Error "RunUAT.bat not found at '$RunUAT'. Check your UE5Root path."
}

Write-Host ""
Write-Host "=== RetroScreen packaging ===" -ForegroundColor Cyan
Write-Host "  Project : $UProjectFile"
Write-Host "  Output  : $OutputDir"
Write-Host "  Config  : $Configuration"
Write-Host "  UE5Root : $UE5Root"
Write-Host ""

# ---------------------------------------------------------------------------
# 1. Copyright cleanliness check — refuse to package if copyrighted assets
#    are present in the project tree.
# ---------------------------------------------------------------------------

Write-Host ">> Checking for copyrighted assets..." -ForegroundColor Yellow

$ForbiddenPatterns = @("*.rom", "*.adf", "*.ipf", "*.hdf")
$ForbiddenFound = @()

foreach ($pattern in $ForbiddenPatterns) {
    $hits = Get-ChildItem -Path $ProjectRoot -Recurse -Filter $pattern -ErrorAction SilentlyContinue |
            Where-Object { $_.FullName -notmatch '\\\.git\\' }
    foreach ($h in $hits) {
        $ForbiddenFound += $h.FullName
    }
}

# Also check for the PUAE core DLL in Plugins — it must not be redistributed
$CoreDll = Join-Path $ProjectRoot "Plugins\UnrealLibretro\MyCores\puae_libretro.dll"
if (Test-Path $CoreDll) {
    $ForbiddenFound += $CoreDll
}

if ($ForbiddenFound.Count -gt 0) {
    Write-Host ""
    Write-Host "ERROR: Copyrighted or non-redistributable files found:" -ForegroundColor Red
    foreach ($f in $ForbiddenFound) {
        Write-Host "  $f" -ForegroundColor Red
    }
    Write-Host ""
    Write-Host "Remove these files before packaging. See LEGAL.md for details." -ForegroundColor Red
    exit 1
}

Write-Host "   No forbidden assets found." -ForegroundColor Green

# ---------------------------------------------------------------------------
# 2. UAT BuildCookRun
# ---------------------------------------------------------------------------

Write-Host ""
Write-Host ">> Running UAT BuildCookRun..." -ForegroundColor Yellow

$StagedBuildsDir = Join-Path $OutputDir "StagedBuilds"

$UATArgs = @(
    "BuildCookRun",
    "-project=`"$UProjectFile`"",
    "-noP4",
    "-platform=Win64",
    "-clientconfig=$Configuration",
    "-cook",
    "-allmaps",
    "-build",
    "-stage",
    "-pak",
    "-archive",
    "-archivedirectory=`"$StagedBuildsDir`""
)

Write-Host "   $RunUAT $($UATArgs -join ' ')"
Write-Host ""

& cmd.exe /c "`"$RunUAT`" $($UATArgs -join ' ')"
if ($LASTEXITCODE -ne 0) {
    Write-Error "UAT BuildCookRun failed with exit code $LASTEXITCODE."
}

Write-Host ""
Write-Host "   Build succeeded." -ForegroundColor Green

# Locate the actual output folder (UAT nests under Win64/)
$BuildRoot = Join-Path $StagedBuildsDir "Win64\RetroScreen"
if (-not (Test-Path $BuildRoot)) {
    # Fallback: the whole StagedBuilds dir
    $BuildRoot = $StagedBuildsDir
}

# ---------------------------------------------------------------------------
# 3. Scaffold empty EmulatorData tree
# ---------------------------------------------------------------------------

Write-Host ""
Write-Host ">> Scaffolding EmulatorData..." -ForegroundColor Yellow

$EmulatorDataDst = Join-Path $BuildRoot "EmulatorData"
$KickstartDst    = Join-Path $EmulatorDataDst "Kickstart"

New-Item -ItemType Directory -Force -Path $KickstartDst | Out-Null

Copy-Item (Join-Path $ProjectRoot "EmulatorData\README.txt")           $EmulatorDataDst -Force
Copy-Item (Join-Path $ProjectRoot "EmulatorData\Kickstart\README.txt") $KickstartDst    -Force

Write-Host "   EmulatorData scaffold written to $EmulatorDataDst" -ForegroundColor Green

# ---------------------------------------------------------------------------
# 4. Copy documentation
# ---------------------------------------------------------------------------

Write-Host ""
Write-Host ">> Copying documentation..." -ForegroundColor Yellow

Copy-Item (Join-Path $ProjectRoot "README.md") $BuildRoot -Force
Copy-Item (Join-Path $ProjectRoot "LEGAL.md")  $BuildRoot -Force

Write-Host "   README.md and LEGAL.md copied." -ForegroundColor Green

# ---------------------------------------------------------------------------
# 5. Summary
# ---------------------------------------------------------------------------

Write-Host ""
Write-Host "=== Package complete ===" -ForegroundColor Cyan
Write-Host "  Output: $BuildRoot"
Write-Host ""
Write-Host "Before distributing, verify:"
Write-Host "  - EmulatorData/ contains only README files (no ROM or ADF)"
Write-Host "  - Plugins/UnrealLibretro/MyCores/ is empty (no puae_libretro.dll)"
Write-Host "  - README.md and LEGAL.md are present in the output root"
Write-Host ""
