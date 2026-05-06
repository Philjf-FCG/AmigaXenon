param(
    [Parameter(Mandatory = $true)]
    [string]$SummaryCsv,
    [string]$OutputPath = "DOCS/RetroScreen_Harness_Summary.md"
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

$resolvedSummaryCsv = Resolve-ProjectPath -Path $SummaryCsv
$resolvedOutputPath = Resolve-ProjectPath -Path $OutputPath

if (-not (Test-Path $resolvedSummaryCsv -PathType Leaf)) {
    throw "Summary CSV not found: $resolvedSummaryCsv"
}

$rows = @(Import-Csv -Path $resolvedSummaryCsv)
if ($rows.Count -eq 0) {
    throw "Summary CSV has no rows."
}

$lines = New-Object System.Collections.Generic.List[string]
$lines.Add("# RetroScreen Harness Run Summary")
$lines.Add("")
$lines.Add(("Generated: {0}" -f (Get-Date -AsUTC).ToString("yyyy-MM-dd HH:mm:ss 'UTC'")))
$lines.Add("")
$lines.Add("| Title | Archetype | Status | VideoFrames | AudioFrames | InputPoll | EffectiveFPS |")
$lines.Add("|-------|-----------|--------|-------------|-------------|-----------|--------------|")

foreach ($row in $rows) {
    if ($row.status -ne "ok" -or -not (Test-Path $row.output_json -PathType Leaf)) {
        $lines.Add(("| {0} | {1} | {2} | - | - | - | - |" -f $row.title, $row.archetype, $row.status))
        continue
    }

    $json = Get-Content -Path $row.output_json -Raw | ConvertFrom-Json
    $lines.Add((
        "| {0} | {1} | {2} | {3} | {4} | {5} | {6:N2} |" -f
        $row.title,
        $row.archetype,
        $row.status,
        $json.video_callback_frames,
        $json.audio_callback_frames,
        $json.input_poll_count,
        [double]$json.effective_fps
    ))
}

$dir = Split-Path -Path $resolvedOutputPath -Parent
if ($dir -and -not (Test-Path $dir)) {
    New-Item -ItemType Directory -Path $dir -Force | Out-Null
}

Set-Content -Path $resolvedOutputPath -Value ($lines -join "`r`n") -Encoding UTF8
Write-Output "Wrote markdown summary: $resolvedOutputPath"
