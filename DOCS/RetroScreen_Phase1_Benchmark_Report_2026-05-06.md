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
- Standalone harness benchmark run: pending
- Phase 2 UE integration instrumentation: available
  - runtime counters for frame publish/consume, texture upload last/avg/max, audio underrun/overrun
  - periodic logging and CSV export pipeline
  - quality-gate evaluator with configurable pass/fail thresholds

## Preliminary Integration Guidance
- Keep emulator execution off the game thread.
- Preserve double-buffered video handoff to reduce contention.
- Track texture upload budget continuously and gate on thresholds.
- Preserve low-latency audio buffering with underrun/overrun counters.

## Open Items To Finalize This Report
1. Execute standalone harness benchmark sessions for PAL and NTSC profiles.
2. Record measured fps, frame-time percentiles, CPU utilization, and audio stability.
3. Attach per-title compatibility table and final pass/fail summary.

## Decision Log
- 2026-05-06: Created benchmark report framework and aligned it to documented TDD targets.
- 2026-05-06: Documented that UE-side instrumentation is now available to accelerate Phase 2 profiling, while Phase 1 harness metrics remain to be captured.
