# RetroScreen Phase 5.1 PIE Smoke Checklist (2026-05-06)

Scope: `AmigaXenon-5qx.6.1` (pause/settings UMG interface)

## Preconditions
- Project builds successfully for `AMigaXenonEditor Win64 Development`.
- Asset exists: `/Game/RetroScreen/UI/WBP_RetroScreenPauseMenu`.
- Manager auto-resolves pause widget class at runtime (or class is set manually in actor details).

## Run Setup
1. Open the project in Unreal Editor.
2. Load the gameplay level used for RetroScreen interaction.
3. Ensure an `ARetroScreenManager` actor is present in the level.
4. Press Play In Editor (PIE).

## Test Cases
1. Open pause menu
- Action: Trigger pause/exit input while in emulator interaction mode.
- Expect: Pause menu appears and emulator pauses.

2. Resume
- Action: Click `Resume`.
- Expect: Pause menu hides, emulator interaction resumes, emulator unpauses.

3. Return to environment
- Action: Open pause menu again, click `Return To Environment`.
- Expect: Pause menu hides, player remains in environment mode, emulator remains paused.

4. CRT toggle
- Action: Toggle `Enable CRT` off/on.
- Expect: Setting is applied without errors and persists to `Saved/RetroScreen.ini`.

5. Audio volume
- Action: Move `Audio Volume` slider to low, medium, and high values.
- Expect: Runtime audio volume updates and persists to `Saved/RetroScreen.ini`.

6. Input mapping flow entry
- Action: Click `Input Mapping`.
- Expect: Pause menu closes and flow returns to environment/input-remap entry state without forcing emulator resume.

7. Input settings
- Action: Change `Joypad Port`, `Joypad Deadzone`, `Map Axes To D-Pad`, `Invert Y Axis`.
- Expect: Input settings apply immediately and persist to `Saved/RetroScreen.ini`.

8. Quit
- Action: Open pause menu and click `Quit`.
- Expect: Quit flow triggers from widget command path (PIE may stop session rather than full desktop exit depending on editor context).

## Completion Criteria
- All test cases pass in a single PIE session with no runtime errors in Output Log.
- Pause menu controls are accessible and functional end-to-end.

## Evidence to Capture
- Output log excerpt for the session.
- Updated `Saved/RetroScreen.ini` keys:
  - `EnableCrt`
  - `RuntimeAudioVolume`
  - `DefaultJoypadPort`
  - `DefaultJoypadDeadzone`
  - `DefaultJoypadMapAxesToDpad`
  - `DefaultJoypadInvertY`
