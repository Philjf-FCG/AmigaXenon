# Libretro Harness

Standalone C++ harness for loading a libretro core DLL, registering frontend callbacks, loading an optional game, and executing a fixed number of `retro_run` frames.

## Build (Windows)

```powershell
Set-Location Tools/LibretroHarness
cmake -S . -B build
cmake --build build --config Release
```

## Run

```powershell
./build/Release/libretro_harness.exe <core_path> [rom_path] [frame_count]
```

Optional flags:

```powershell
--option key=value    # set libretro core variable
--kickstart <path>    # convenience option mapping for puae_rom and kickstart_path
--rom <path>          # explicit ROM/ADF path
--frames <N>          # number of retro_run iterations
--output <path>       # write JSON run summary
```

Examples:

```powershell
./build/Release/libretro_harness.exe "C:/path/to/puae_libretro.dll" "C:/path/to/game.adf" 600
./build/Release/libretro_harness.exe "C:/path/to/puae_libretro.dll"
./build/Release/libretro_harness.exe "C:/path/to/puae_libretro.dll" --kickstart "C:/path/to/kick.rom" --rom "C:/path/to/game.adf" --frames 600 --option puae_model=a500
./build/Release/libretro_harness.exe "C:/path/to/puae_libretro.dll" --rom "C:/path/to/game.adf" --frames 600 --output "C:/tmp/harness_run.json"
```

## Notes

- This harness focuses on callback wiring and frame execution validation.
- Video and audio callbacks collect metrics only; no window/audio device output is created.
- Input callbacks currently return neutral state (no buttons pressed).
