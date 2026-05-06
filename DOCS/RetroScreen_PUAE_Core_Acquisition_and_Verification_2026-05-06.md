# PUAE Core Acquisition and Verification

Date: 2026-05-06
Issue: AmigaXenon-5qx.1.2

## Goal
Obtain a compatible PUAE libretro core binary for local testing, place it in expected project paths, and verify it loads through both standalone harness and UE manager integration.

## Expected Binary (Windows)
- Filename pattern: puae*_libretro.dll
- Recommended local destination:
  - Plugins/UnrealLibretro/MyCores/

## Acquisition Checklist
1. Download from trusted source (official libretro build infrastructure or equivalent trusted mirror).
2. Record download URL and timestamp.
3. Record local file size and SHA256 hash.
4. Confirm architecture matches host (x64 for Win64).

## Verification Checklist
1. File presence
- Confirm DLL exists at configured path.

2. Symbol sanity
- Validate required retro symbols are present via harness startup:
  - retro_set_environment
  - retro_set_video_refresh
  - retro_set_audio_sample_batch
  - retro_set_input_poll
  - retro_set_input_state
  - retro_init / retro_deinit
  - retro_load_game / retro_unload_game
  - retro_run

3. Harness smoke run
- Run libretro_harness with core path and optional ROM path.
- Confirm non-zero callback counters (video/audio/input where applicable).

4. UE manager smoke run
- Configure Saved/RetroScreen.ini with LibretroCorePath and LibretroRomPath.
- Confirm ARetroScreenManager initialization succeeds and runtime metrics update.

## Logging Template
- Core path:
- Source URL:
- SHA256:
- File size:
- Harness result:
- UE result:
- Notes:

## Licensing Note
- Keep core licensing records alongside project documentation.
- Do not bundle copyrighted game media or Kickstart ROMs.
