# RetroScreen Phase 1 Benchmark Report

Date: 2026-05-06
Issue: AmigaXenon-5qx.2.8
Owner: FCG-build

## Scope
Phase 1 objective is to validate standalone libretro Amiga core operation outside Unreal Engine and capture compatibility/performance evidence used to guide Unreal integration decisions.

## Target Metrics
- Emulator frame rate: 50 fps (PAL) or 60 fps (NTSC)
- Frame pacing: stable cadence with no sustained frame drops
- Audio quality: no persistent underruns/stutter during representative gameplay
- CPU cost: acceptable sustained utilization on a mid-range desktop profile

## Test Matrix
- Core: PUAE libretro
- Kickstart: user-supplied legal ROM
- Workloads:
  - keyboard-centric title
  - mouse-centric title
  - joystick-centric title

## Measurement Plan
- Timing capture:
  - per-frame delta and 95th/99th percentile frame-time
  - effective emulation fps over 5 minute windows
- CPU capture:
  - process CPU percent average and peak over 5 minute windows
- Audio capture:
  - callback underrun/overrun counts and audible artifact notes
- Compatibility capture:
  - boot success, input functionality, and stability observations by title

## Current Evidence Status
- Standalone harness benchmark run: complete
  - Harness executable: `Tools/LibretroHarness/build/Release/libretro_harness.exe`
  - Run artifacts: `Saved/HarnessRuns/*.json`
  - Aggregated markdown: `DOCS/RetroScreen_Harness_Summary.md`
- Phase 2 UE integration instrumentation: available
  - runtime counters for frame publish/consume, texture upload last/avg/max, audio underrun/overrun
  - periodic logging and CSV export pipeline
  - quality-gate evaluator with configurable pass/fail thresholds

## Benchmark Results (Phase 1)
Run profile:
- Frames per case: 1800
- Core: `Plugins/UnrealLibretro/MyCores/puae_libretro.dll`
- Cases: XenonDisk1 (keyboard), XenonDisk2 (mouse), Barbarian (joystick)

| Title | Archetype | Status | Video Frames | Audio Frames | Input Polls | Effective FPS | Frame P95 (ms) | Frame P99 (ms) | CPU Avg (%) |
|-------|-----------|--------|--------------|--------------|-------------|---------------|----------------|----------------|-------------|
| XenonDisk1 | keyboard | ok | 1800 | 1593296 | 1800 | 644.29 | 2.052 | 2.769 | 4.70 |
| XenonDisk2 | mouse | ok | 1800 | 1588268 | 1800 | 635.57 | 1.996 | 2.441 | 4.91 |
| Barbarian | joystick | ok | 1800 | 1593296 | 1800 | 661.96 | 1.808 | 2.362 | 4.88 |

Interpretation:
- All three cataloged archetypes booted and ran stably for the benchmark window.
- Callback telemetry was consistent (video/input counts matched requested frame counts).
- CPU usage remained low on this host during uncapped harness execution.
- Effective FPS exceeded PAL/NTSC playback targets because this harness run is not frame-capped; these results are throughput/stability evidence rather than real-time pacing validation.

## Preliminary Integration Guidance
- Keep emulator execution off the game thread.
- Preserve double-buffered video handoff to reduce contention.
- Track texture upload budget continuously and gate on thresholds.
- Preserve low-latency audio buffering with underrun/overrun counters.

## Open Items To Finalize This Report
1. Add optional frame-cap mode to harness for strict 50/60 fps pacing verification.
2. Extend report with per-title qualitative input behavior notes from interactive sessions.

## Decision Log
- 2026-05-06: Created benchmark report framework and aligned it to documented TDD targets.
- 2026-05-06: Documented that UE-side instrumentation is now available to accelerate Phase 2 profiling, while Phase 1 harness metrics remain to be captured.
- 2026-05-06: Completed standalone benchmark runs for keyboard/mouse/joystick archetypes and published metrics table from automated harness outputs.
