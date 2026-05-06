# RetroScreen Architecture and Coding Conventions

Date: 2026-05-06
Issue: AmigaXenon-5qx.1.5

## Architecture Overview

### Runtime Layers
1. Manager Layer
- ARetroScreenManager orchestrates startup/shutdown, mode selection, and telemetry.
- Supports two runtime modes:
  - UnrealLibretro component path (ULibretroCoreInstance)
  - Standalone dynamic core path (FRetroScreenLibretroCore)

2. Emulation Execution Layer
- FRetroScreenEmulatorWorker runs on FRunnableThread at target cadence.
- Worker executes either:
  - real retro_run() via FRetroScreenLibretroCore when configured
  - synthetic stepping fallback for pipeline validation when no core is configured

3. Callback Bridge Layer
- FRetroScreenInputBridge handles libretro-style poll/state input semantics.
- FRetroScreenVideoBridge handles frame publication/consumption with double buffering.
- FRetroScreenAudioBridge handles PCM transport via ring buffer.
- FRetroScreenTextureBridge performs frame upload to transient UTexture2D.

4. Metrics and Quality Layer
- Runtime metrics include frame publish/consume, upload timings, audio counters, input poll count, and worker timing.
- Quality gate evaluator performs threshold checks with pass/fail summary.
- Metrics can be logged periodically and exported to CSV.

### Standalone Harness
- Tools/LibretroHarness provides non-UE callback wiring and frame execution validation.
- It dynamically loads core symbols, registers callbacks, optionally loads game path, and runs a fixed frame count.

## Threading and Ownership Rules
- Emulator core stepping occurs on worker thread only.
- Shared metrics are protected by FCriticalSection in manager.
- Input bridge uses RW lock and atomic poll counter.
- Audio ring buffer exposes producer/consumer metrics for underrun/overrun analysis.

## Configuration Rules
- Runtime configuration is loaded from RetroScreen.ini via manager at BeginPlay.
- Core paths, mode selection, telemetry toggles, and quality thresholds are config-driven.
- Relative paths are resolved against project directory.

## Coding Conventions
1. Naming
- Public runtime types use RetroScreen prefix.
- UE-facing actor/API symbols use ARetroScreen* and Blueprint categories under RetroScreen.

2. Error Handling
- Fail fast on missing core/ROM/symbol dependencies with explicit warning logs.
- Return boolean status for lifecycle operations where startup can fail.

3. Telemetry
- Any new bridge or execution path should contribute observable counters.
- CSV schema changes must be mirrored in summarization tooling.

4. Backward Safety
- Real-core path should keep synthetic fallback available for local validation when assets are absent.
- Changes to runtime config keys should include defaults and clamping for intervals.

5. File and Module Boundaries
- Keep worker, bridges, loader, and manager in separate translation units.
- Avoid embedding dynamic loading logic directly in bridge classes; keep in loader wrapper.

## Immediate Next Extensions
- Replace synthetic fallback callbacks with full standalone loader callback registration against core-provided AV negotiation where available.
- Add symbol coverage for serialization/save-state and memory read/write operations.
- Add automated smoke test script once CMake/toolchain dependency is available on build machine.
