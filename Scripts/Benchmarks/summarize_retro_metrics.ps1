param(
    [string]$CsvPath = "Saved/RetroScreenMetrics.csv",
    [string]$OutputPath = ""
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

$resolvedCsvPath = Resolve-ProjectPath -Path $CsvPath
if (-not (Test-Path -Path $resolvedCsvPath -PathType Leaf)) {
    throw "Metrics CSV not found at '$resolvedCsvPath'."
}

$rows = @(Import-Csv -Path $resolvedCsvPath)
if ($rows.Count -eq 0) {
    throw "Metrics CSV is empty: '$resolvedCsvPath'."
}

$first = $rows[0]
$last = $rows[$rows.Count - 1]

function To-Double {
    param([object]$Value)
    return [double]::Parse($Value.ToString(), [System.Globalization.CultureInfo]::InvariantCulture)
}

function To-Int64 {
    param([object]$Value)
    return [int64]::Parse($Value.ToString(), [System.Globalization.CultureInfo]::InvariantCulture)
}

function Has-Column {
    param(
        [object]$Row,
        [string]$Name
    )

    return $null -ne $Row.PSObject.Properties[$Name]
}

$lastUploadValues = @($rows | ForEach-Object { To-Double $_.last_texture_upload_ms })
$avgUploadValues = @($rows | ForEach-Object { To-Double $_.avg_texture_upload_ms })
$maxUploadValues = @($rows | ForEach-Object { To-Double $_.max_texture_upload_ms })

$lastUploadMean = ($lastUploadValues | Measure-Object -Average).Average
$avgUploadMean = ($avgUploadValues | Measure-Object -Average).Average
$maxUploadPeak = ($maxUploadValues | Measure-Object -Maximum).Maximum

$firstFramesPublished = To-Int64 $first.video_frames_published
$lastFramesPublished = To-Int64 $last.video_frames_published
$firstFramesConsumed = To-Int64 $first.video_frames_consumed
$lastFramesConsumed = To-Int64 $last.video_frames_consumed

$firstUnderrun = To-Int64 $first.audio_underrun_samples
$lastUnderrun = To-Int64 $last.audio_underrun_samples
$firstOverrun = To-Int64 $first.audio_overrun_samples
$lastOverrun = To-Int64 $last.audio_overrun_samples

$framePublishDelta = $lastFramesPublished - $firstFramesPublished
$frameConsumeDelta = $lastFramesConsumed - $firstFramesConsumed
$underrunDelta = $lastUnderrun - $firstUnderrun
$overrunDelta = $lastOverrun - $firstOverrun

$inputPollSection = ""
if (Has-Column -Row $last -Name "input_poll_count") {
    $inputPollFirst = To-Int64 $first.input_poll_count
    $inputPollLast = To-Int64 $last.input_poll_count
    $inputPollDelta = $inputPollLast - $inputPollFirst

    $inputPollSection = @"

## Input Polling
- Input poll delta: $inputPollDelta
- Final input poll count: $inputPollLast
"@
}

$hasWorkerColumns = Has-Column -Row $last -Name "worker_frames_executed"
$workerSection = ""
if ($hasWorkerColumns) {
    $workerFrameFirst = To-Int64 $first.worker_frames_executed
    $workerFrameLast = To-Int64 $last.worker_frames_executed
    $workerFrameDelta = $workerFrameLast - $workerFrameFirst

    $workerLastValues = @($rows | ForEach-Object { To-Double $_.worker_last_frame_ms })
    $workerAvgValues = @($rows | ForEach-Object { To-Double $_.worker_avg_frame_ms })
    $workerMaxValues = @($rows | ForEach-Object { To-Double $_.worker_max_frame_ms })

    $workerLastMean = ($workerLastValues | Measure-Object -Average).Average
    $workerAvgMean = ($workerAvgValues | Measure-Object -Average).Average
    $workerMaxPeak = ($workerMaxValues | Measure-Object -Maximum).Maximum

    $workerSection = @"

## Worker Thread Timing
- Worker frame delta: $workerFrameDelta
- Mean worker last-frame time (ms): $([math]::Round($workerLastMean, 4))
- Mean worker running-average frame time (ms): $([math]::Round($workerAvgMean, 4))
- Peak worker max-frame time (ms): $([math]::Round($workerMaxPeak, 4))
"@
}

$utcNow = Get-Date -AsUTC
$headerDate = $utcNow.ToString("yyyy-MM-dd")
$headerTime = $utcNow.ToString("yyyy-MM-dd HH:mm:ss 'UTC'")

if ([string]::IsNullOrWhiteSpace($OutputPath)) {
    $defaultName = "RetroScreen_UE_Metrics_Summary_{0}.md" -f $utcNow.ToString("yyyyMMdd_HHmmss")
    $OutputPath = Join-Path "DOCS" $defaultName
}

$resolvedOutputPath = Resolve-ProjectPath -Path $OutputPath
$resolvedOutputDir = Split-Path -Path $resolvedOutputPath -Parent
if (-not [string]::IsNullOrWhiteSpace($resolvedOutputDir) -and -not (Test-Path $resolvedOutputDir)) {
    New-Item -ItemType Directory -Path $resolvedOutputDir -Force | Out-Null
}

$report = @"
# RetroScreen UE Metrics Summary

Generated: $headerTime
Source CSV: $resolvedCsvPath

## Sample Window
- Rows: $($rows.Count)
- First timestamp (UTC): $($first.timestamp_utc)
- Last timestamp (UTC): $($last.timestamp_utc)

## Video Counters
- Published frame delta: $framePublishDelta
- Consumed frame delta: $frameConsumeDelta
- Last sequence: $(To-Int64 $last.last_video_frame_sequence)

## Texture Upload Timing (ms)
- Mean of last upload time: $([math]::Round($lastUploadMean, 4))
- Mean of running average: $([math]::Round($avgUploadMean, 4))
- Peak observed max upload time: $([math]::Round($maxUploadPeak, 4))
- Final running average (last row): $([math]::Round((To-Double $last.avg_texture_upload_ms), 4))
- Final running max (last row): $([math]::Round((To-Double $last.max_texture_upload_ms), 4))

## Audio Stability
- Underrun delta: $underrunDelta
- Overrun delta: $overrunDelta
- Final buffered samples: $(To-Int64 $last.audio_buffered_samples)
- Final pushed samples: $(To-Int64 $last.audio_samples_pushed)
- Final popped samples: $(To-Int64 $last.audio_samples_popped)
$inputPollSection
$workerSection

## Interpretation Notes
- Use this summary as input to Sprint 1 profiling and optimization decisions.
- Compare peak upload values against active quality-gate thresholds.
- Non-zero underrun/overrun deltas indicate buffer tuning or scheduling pressure.
"@

Set-Content -Path $resolvedOutputPath -Value $report -Encoding UTF8

Write-Output "Wrote metrics summary: $resolvedOutputPath"
