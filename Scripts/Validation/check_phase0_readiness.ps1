param(
    [string]$ProjectRoot = ".",
    [string]$OutputPath = "DOCS/logs/phase0_readiness_latest.txt"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-ProjectPath {
    param([string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }

    return Join-Path (Get-Location) $Path
}

function New-CheckResult {
    param(
        [string]$Name,
        [bool]$Passed,
        [string]$Details
    )

    return [pscustomobject]@{
        Name = $Name
        Passed = $Passed
        Details = $Details
    }
}

$resolvedRoot = Resolve-ProjectPath -Path $ProjectRoot
Set-Location $resolvedRoot

$results = New-Object System.Collections.Generic.List[object]

$unrealBuildToolCommand = Get-Command UnrealBuildTool -ErrorAction SilentlyContinue
$unrealEditorCommand = Get-Command UnrealEditor -ErrorAction SilentlyContinue

$candidateEngineRoots = @()
$epicGamesRoots = @(
    "C:\Program Files\Epic Games",
    "D:\Epic Games"
)

foreach ($epicRoot in $epicGamesRoots) {
    if (Test-Path $epicRoot) {
        $candidateEngineRoots += Get-ChildItem -Path $epicRoot -Directory -Filter "UE_*" -ErrorAction SilentlyContinue
    }
}

$selectedEngineRoot = $candidateEngineRoots |
    Sort-Object -Property Name -Descending |
    Select-Object -First 1

$buildBatFromEngine = $null
$editorFromEngine = $null
if ($null -ne $selectedEngineRoot) {
    $buildCandidate = Join-Path $selectedEngineRoot.FullName "Engine\Build\BatchFiles\Build.bat"
    $editorCandidate = Join-Path $selectedEngineRoot.FullName "Engine\Binaries\Win64\UnrealEditor.exe"

    if (Test-Path $buildCandidate -PathType Leaf) {
        $buildBatFromEngine = $buildCandidate
    }

    if (Test-Path $editorCandidate -PathType Leaf) {
        $editorFromEngine = $editorCandidate
    }
}

$buildToolAvailable = ($null -ne $unrealBuildToolCommand) -or ($null -ne $buildBatFromEngine)
$editorAvailable = ($null -ne $unrealEditorCommand) -or ($null -ne $editorFromEngine)

$unrealBuildToolDetails = if ($null -ne $unrealBuildToolCommand) {
    $unrealBuildToolCommand.Source
}
elseif ($null -ne $buildBatFromEngine) {
    "Build.bat found at $buildBatFromEngine"
}
else {
    "Not found in PATH or common engine locations"
}

$unrealEditorDetails = if ($null -ne $unrealEditorCommand) {
    $unrealEditorCommand.Source
}
elseif ($null -ne $editorFromEngine) {
    $editorFromEngine
}
else {
    "Not found in PATH or common engine locations"
}

$results.Add((New-CheckResult -Name "UnrealBuildTool available" -Passed $buildToolAvailable -Details $unrealBuildToolDetails))
$results.Add((New-CheckResult -Name "UnrealEditor available" -Passed $editorAvailable -Details $unrealEditorDetails))

$uprojectFiles = @(Get-ChildItem -Path $resolvedRoot -File -Filter "*.uproject" -ErrorAction SilentlyContinue)
$uprojectDetails = if ($uprojectFiles.Count -gt 0) {
    ($uprojectFiles | ForEach-Object { $_.FullName }) -join "; "
}
else {
    "Count=0"
}

$results.Add((New-CheckResult -Name "UE project file present" -Passed ($uprojectFiles.Count -gt 0) -Details $uprojectDetails))

$targetFiles = @(Get-ChildItem -Path $resolvedRoot -Recurse -File -Filter "*Target.cs" -ErrorAction SilentlyContinue)
$results.Add((New-CheckResult -Name "UE target files present" -Passed ($targetFiles.Count -gt 0) -Details ("Count={0}" -f $targetFiles.Count)))

$coreCandidates = @(Get-ChildItem -Path (Join-Path $resolvedRoot "Plugins/UnrealLibretro") -Recurse -File -Filter "*puae*_libretro.dll" -ErrorAction SilentlyContinue)
$results.Add((New-CheckResult -Name "PUAE core DLL present" -Passed ($coreCandidates.Count -gt 0) -Details ("Count={0}" -f $coreCandidates.Count)))

$kickstartCandidates = @(Get-ChildItem -Path (Join-Path $resolvedRoot "EmulatorData/Kickstart") -Recurse -File -Include "*.rom","*.bin" -ErrorAction SilentlyContinue)
$results.Add((New-CheckResult -Name "Kickstart ROM present" -Passed ($kickstartCandidates.Count -gt 0) -Details ("Count={0}" -f $kickstartCandidates.Count)))

$adfCandidates = @(Get-ChildItem -Path (Join-Path $resolvedRoot "EmulatorData/ADF") -Recurse -File -Filter "*.adf" -ErrorAction SilentlyContinue)
$results.Add((New-CheckResult -Name "ADF test set (>=3) present" -Passed ($adfCandidates.Count -ge 3) -Details ("Count={0}" -f $adfCandidates.Count)))

$runtimeConfigPath = Join-Path $resolvedRoot "Saved/RetroScreen.ini"
$results.Add((New-CheckResult -Name "Runtime config present" -Passed (Test-Path $runtimeConfigPath -PathType Leaf) -Details $runtimeConfigPath))

$archDoc = Join-Path $resolvedRoot "DOCS/RetroScreen_Architecture_and_Conventions_2026-05-06.md"
$results.Add((New-CheckResult -Name "Architecture/conventions doc present" -Passed (Test-Path $archDoc -PathType Leaf) -Details $archDoc))

$puaeDoc = Join-Path $resolvedRoot "DOCS/RetroScreen_PUAE_Core_Acquisition_and_Verification_2026-05-06.md"
$results.Add((New-CheckResult -Name "PUAE acquisition doc present" -Passed (Test-Path $puaeDoc -PathType Leaf) -Details $puaeDoc))

$kickDoc = Join-Path $resolvedRoot "DOCS/RetroScreen_Kickstart_ROM_Provisioning_Checklist_2026-05-06.md"
$results.Add((New-CheckResult -Name "Kickstart checklist doc present" -Passed (Test-Path $kickDoc -PathType Leaf) -Details $kickDoc))

$catalogDoc = Join-Path $resolvedRoot "DOCS/RetroScreen_Harness_Catalog_template.csv"
$results.Add((New-CheckResult -Name "Harness catalog template present" -Passed (Test-Path $catalogDoc -PathType Leaf) -Details $catalogDoc))

$failedCount = @($results | Where-Object { -not $_.Passed }).Count
$allPassed = $failedCount -eq 0

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add(("Phase 0 readiness report - {0}" -f (Get-Date -AsUTC).ToString("yyyy-MM-dd HH:mm:ss 'UTC'")))
$lines.Add(("Root: {0}" -f $resolvedRoot))
$lines.Add("")

foreach ($result in $results) {
    $status = if ($result.Passed) { "PASS" } else { "FAIL" }
    $lines.Add(("[{0}] {1} :: {2}" -f $status, $result.Name, $result.Details))
}

$lines.Add("")
$overallStatus = if ($allPassed) { "PASS" } else { "FAIL" }
$lines.Add(("OVERALL: {0}" -f $overallStatus))

$resolvedOutputPath = Resolve-ProjectPath -Path $OutputPath
$outputDir = Split-Path -Path $resolvedOutputPath -Parent
if (-not [string]::IsNullOrWhiteSpace($outputDir) -and -not (Test-Path $outputDir)) {
    New-Item -ItemType Directory -Path $outputDir -Force | Out-Null
}

Set-Content -Path $resolvedOutputPath -Value ($lines -join [Environment]::NewLine) -Encoding UTF8

$lines | ForEach-Object { Write-Output $_ }

if (-not $allPassed) {
    exit 2
}

exit 0