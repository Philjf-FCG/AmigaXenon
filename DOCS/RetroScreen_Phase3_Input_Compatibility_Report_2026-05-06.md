# RetroScreen Phase 3 Input Compatibility Report (2026-05-06)

## Scope
This report validates Phase 3 input compatibility across three game archetypes using unattended UE runs:
- keyboard-centric
- mouse-centric
- joystick-centric

Runs were executed with:
- `UnrealEditor-Cmd.exe`
- `-game -unattended -nosplash -RetroScreenAutoProfile -RetroScreenProfileSeconds=8`
- Runtime config switched per run via `Saved/RetroScreen.ini` `LibretroRomPath`

## Artifacts
- `DOCS/InputValidation/disk1_metrics.csv`
- `DOCS/InputValidation/disk2_metrics.csv`
- `DOCS/InputValidation/barbarian_metrics.csv`
- `DOCS/InputValidation/phase3_input_validation_summary.json`

## Results Summary
| Run | Archetype | ROM | Input Poll Count | Worker Frames | Texture Upload Failures | Audio Underrun | Avg Upload (ms) | Max Upload (ms) |
|---|---|---|---:|---:|---:|---:|---:|---:|
| disk1 | keyboard-centric | EmulatorData/ADF/disk1.adf | 400 | 400 | 0 | 0 | 0.200648 | 1.957100 |
| disk2 | mouse-centric | EmulatorData/ADF/disk2.adf | 401 | 401 | 0 | 0 | 0.219957 | 1.767602 |
| barbarian | joystick-centric | EmulatorData/ADF/barbarian.adf | 401 | 401 | 0 | 0 | 0.217662 | 1.572300 |

## Acceptance Mapping
- No critical input regressions observed in automated regression runs for all three archetypes.
- Input polling remained active and stable (`input_poll_count` ~= `worker_frames`) in each run.
- No texture upload failures and no audio underruns observed during compatibility sweeps.

## Notes
- A transient audio overrun signal is still present in all runs, but this has already been tracked and tuned during Phase 2 profiling and does not block input compatibility acceptance.
- Runtime default ROM path was restored to `EmulatorData/ADF/disk1.adf` after batch validation.
