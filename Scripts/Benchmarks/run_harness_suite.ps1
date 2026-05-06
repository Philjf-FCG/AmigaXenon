param(
    [Parameter(Mandatory = $true)]
    [string]$HarnessExe,

    [Parameter(Mandatory = $true)]
    [string]$CorePath,

    [Parameter(Mandatory = $true)]
    [string]$CatalogCsv,

    [string]$OutputDir = "Saved/HarnessRuns",
    [int]$Frames = 600
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

$resolvedHarnessExe = Resolve-ProjectPath -Path $HarnessExe
$resolvedCorePath = Resolve-ProjectPath -Path $CorePath
$resolvedCatalogCsv = Resolve-ProjectPath -Path $CatalogCsv
$resolvedOutputDir = Resolve-ProjectPath -Path $OutputDir

if (-not (Test-Path $resolvedHarnessExe -PathType Leaf)) {
    throw "Harness executable not found: $resolvedHarnessExe"
}

if (-not (Test-Path $resolvedCorePath -PathType Leaf)) {
    throw "Core file not found: $resolvedCorePath"
}

if (-not (Test-Path $resolvedCatalogCsv -PathType Leaf)) {
    throw "Catalog CSV not found: $resolvedCatalogCsv"
}

if (-not (Test-Path $resolvedOutputDir)) {
    New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
}

$cases = @(Import-Csv -Path $resolvedCatalogCsv)
if ($cases.Count -eq 0) {
    throw "Catalog CSV has no rows: $resolvedCatalogCsv"
}

$results = @()

foreach ($case in $cases) {
    $title = $case.title
    $archetype = $case.archetype
    $romPath = Resolve-ProjectPath -Path $case.rom_path

    if (-not (Test-Path $romPath -PathType Leaf)) {
        $results += [pscustomobject]@{
            title = $title
            archetype = $archetype
            rom_path = $romPath
            status = "missing-rom"
            output_json = ""
        }
        continue
    }

    $safeName = ($title -replace '[^A-Za-z0-9_-]', '_')
    $jsonOut = Join-Path $resolvedOutputDir ("{0}.json" -f $safeName)

    $args = @(
        $resolvedCorePath,
        "--rom", $romPath,
        "--frames", $Frames.ToString(),
        "--output", $jsonOut
    )

    if ($case.kickstart_path -and -not [string]::IsNullOrWhiteSpace($case.kickstart_path)) {
        $kickstartPath = Resolve-ProjectPath -Path $case.kickstart_path
        $args += @("--kickstart", $kickstartPath)
    }

    & $resolvedHarnessExe @args
    $exitCode = $LASTEXITCODE

    $results += [pscustomobject]@{
        title = $title
        archetype = $archetype
        rom_path = $romPath
        status = $(if ($exitCode -eq 0) { "ok" } else { "failed" })
        output_json = $jsonOut
    }
}

$summaryPath = Join-Path $resolvedOutputDir "harness_suite_summary.csv"
$results | Export-Csv -Path $summaryPath -NoTypeInformation -Encoding UTF8
Write-Output "Wrote harness suite summary: $summaryPath"
