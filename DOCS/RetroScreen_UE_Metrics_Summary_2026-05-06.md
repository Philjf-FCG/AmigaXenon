# RetroScreen UE Metrics Summary

Generated: 2026-05-06 13:07:58 UTC
Source CSV: C:\DevProjects\AmigaXenon\Saved\RetroScreenMetrics.csv

## Sample Window
- Rows: 4
- First timestamp (UTC): 2026-05-06T13:07:18Z
- Last timestamp (UTC): 2026-05-06T13:07:28Z

## Video Counters
- Published frame delta: 501
- Consumed frame delta: 200
- Last sequence: 501

## Texture Upload Timing (ms)
- Mean of last upload time: 0.1326
- Mean of running average: 0.2361
- Peak observed max upload time: 43.9846
- Final running average (last row): 0.4505
- Final running max (last row): 43.9846

## Audio Stability
- Underrun delta: 0
- Overrun delta: 352000
- Final buffered samples: 1920
- Final pushed samples: 609920
- Final popped samples: 608000

## Input Polling
- Input poll delta: 501
- Final input poll count: 501

## Worker Thread Timing
- Worker frame delta: 501
- Mean worker last-frame time (ms): 0.9496
- Mean worker running-average frame time (ms): 0.7218
- Peak worker max-frame time (ms): 5.2865

## Interpretation Notes
- Use this summary as input to Sprint 1 profiling and optimization decisions.
- Compare peak upload values against active quality-gate thresholds.
- Non-zero underrun/overrun deltas indicate buffer tuning or scheduling pressure.
