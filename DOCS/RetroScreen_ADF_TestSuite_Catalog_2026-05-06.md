# RetroScreen ADF Test Suite Catalog

Date: 2026-05-06
Issue: AmigaXenon-5qx.1.4

## Purpose
Define a baseline, legally sourced Amiga test suite spanning input archetypes (keyboard, mouse, joystick) for repeatable compatibility and regression validation.

## Storage Convention
- Local media root (not committed): EmulatorData/
- Suggested subfolders:
  - EmulatorData/ADF/Keyboard/
  - EmulatorData/ADF/Mouse/
  - EmulatorData/ADF/Joystick/

## Minimum Coverage Requirement
- At least 3 representative ADF titles:
  - 1 keyboard-centric
  - 1 mouse-centric
  - 1 joystick-centric

## Catalog Template
| Slot | Archetype | Title | Region | Source Owner | Local Relative Path | Kickstart Requirement | Booted In Harness | Booted In UE | Notes |
|------|-----------|-------|--------|--------------|---------------------|-----------------------|-------------------|--------------|-------|
| 1 | Keyboard | TBD | PAL/NTSC | User-supplied | EmulatorData/ADF/Keyboard/TBD.adf | TBD | No | No | |
| 2 | Mouse | TBD | PAL/NTSC | User-supplied | EmulatorData/ADF/Mouse/TBD.adf | TBD | No | No | |
| 3 | Joystick | TBD | PAL/NTSC | User-supplied | EmulatorData/ADF/Joystick/TBD.adf | TBD | No | No | |

## Validation Checklist
1. Confirm each ADF file exists at the cataloged local path.
2. Confirm legal ownership/source attribution is documented externally by user.
3. Verify each title boots in standalone harness path.
4. Verify each title reaches first interactive screen in UE integration path.
5. Capture per-title input mapping notes and anomalies.

## Completion Criteria for This Task
- Three baseline ADF entries are filled with concrete titles and local paths.
- Each entry includes input archetype and boot status placeholders.
- Catalog is referenced by Sprint profiling and compatibility tasks.
