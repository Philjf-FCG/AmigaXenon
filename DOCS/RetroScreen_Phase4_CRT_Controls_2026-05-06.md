# RetroScreen Phase 4 CRT Controls (2026-05-06)

## Scope
This update implements configurable CRT-effect control parameters on the cabinet screen material pipeline.

Implemented controls:
- Scanline intensity
- Curvature
- Phosphor bloom
- Vignette
- Chromatic aberration

## Runtime API
The `ARetroScreenCabinetActor` now exposes Blueprint-callable APIs for runtime tuning:
- `SetCrtParameters(const FRetroScreenCrtParameters&)`
- `SetCrtEnabled(bool)`
- `GetCrtParameters()`
- `IsCrtEnabled()`

These can be bound by gameplay UI/pause-menu widgets to adjust CRT quality and accessibility preferences while running.

## Material Parameter Contract
The dynamic screen material instance receives these scalar parameters:
- `CRT_Enabled`
- `CRT_ScanlineIntensity`
- `CRT_Curvature`
- `CRT_PhosphorBloom`
- `CRT_Vignette`
- `CRT_ChromaticAberration`

Screen texture parameter remains:
- `ScreenTexture`

## Implementation Notes
- CRT values are clamped in code for stable runtime tuning.
- Parameter application is dirty-flagged to avoid redundant writes every tick.
- Existing cabinet integration supports both:
  - dedicated screen mesh material mode
  - single-mesh screen material-slot mode

## Validation
- UE 5.7 command-line build succeeded after implementing CRT controls.
