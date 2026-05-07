# RetroScreen
### Unreal Engine 5 · Commodore Amiga Arcade Emulator

RetroScreen places a fully functional Commodore Amiga emulator inside an interactive 3D arcade cabinet. The cabinet's screen runs a real Amiga game; your keyboard, mouse, or gamepad input passes through to the emulator in real time.

> **Legal notice** — RetroScreen requires files you must supply yourself (Kickstart ROM, game disk). It does **not** include or distribute any copyrighted Amiga ROM or software. See [LEGAL.md](LEGAL.md) before proceeding.

---

## System Requirements

| | Minimum | Recommended |
|---|---|---|
| **OS** | Windows 10 64-bit | Windows 11 64-bit |
| **CPU** | 4-core / 3 GHz | 6-core / 3.5 GHz |
| **GPU** | DX12-capable, 4 GB VRAM | RTX 2060 / RX 6600 or better |
| **RAM** | 8 GB | 16 GB |
| **Storage** | 2 GB free | SSD recommended |

---

## Quick Start

### 1 — Provide a Kickstart ROM

The emulator requires an original Amiga Kickstart ROM image. You must obtain this legally (see [LEGAL.md](LEGAL.md)).

Place your `.rom` file in:

```
EmulatorData/
  Kickstart/
    kick13.rom        ← any filename, extension must be .rom
```

The emulator will use the first `.rom` file it finds in that folder.  
Kickstart 1.3 (A500) or 3.1 (A1200) are recommended for broad game compatibility.

### 2 — Provide a game disk

Place your `.adf` game disk image anywhere you like, then point the config at it:

```
Saved/
  RetroScreen.ini     ← edit this file
```

Set `LibretroRomPath` to the path of your ADF (relative to the project folder, or absolute):

```ini
[RetroScreen]
LibretroRomPath=EmulatorData/MyGame.adf
```

### 3 — Provide the emulator core DLL

The PUAE libretro core DLL must be placed at:

```
Plugins/UnrealLibretro/MyCores/puae_libretro.dll
```

You can download a Windows build of the PUAE core from the libretro buildbot:  
`https://buildbot.libretro.com/nightly/windows/x86_64/latest/`  
Look for `puae_libretro.dll` in the archive.

### 4 — Run the game

Launch `RetroScreen.exe`. If any required file is missing, a setup guide will appear on-screen explaining what to add. Once all files are present, the arcade cabinet will start automatically.

---

## Directory Structure

```
RetroScreen/
├── RetroScreen.exe
├── README.md
├── LEGAL.md
├── EmulatorData/               ← create this folder; add your files here
│   ├── Kickstart/
│   │   └── kick13.rom          ← your Kickstart ROM
│   └── MyGame.adf              ← your game disk (path set in RetroScreen.ini)
├── Saved/
│   └── RetroScreen.ini         ← runtime configuration (auto-created on first run)
└── Plugins/
    └── UnrealLibretro/
        └── MyCores/
            └── puae_libretro.dll  ← PUAE emulator core
```

---

## Controls

### Environment (walking around the room)

| Input | Action |
|---|---|
| **W A S D** | Move |
| **Mouse** | Look |
| **E** / **Gamepad A** | Interact with cabinet |
| **Escape** | Menu |

### Emulator (playing the Amiga game)

Pressing **E** near the cabinet switches to emulator mode. The camera moves to the screen.

| Input | Amiga equivalent |
|---|---|
| **Keyboard** | Full Amiga keyboard (all keys forwarded) |
| **Mouse movement** | Amiga mouse |
| **Left mouse button** | Amiga left mouse button |
| **Right mouse button** | Amiga right mouse button |
| **Gamepad left stick / D-pad** | Joystick port 1 |
| **Gamepad A / Cross** | Joystick fire button |
| **Gamepad B / Circle** | Second fire / mouse button 2 |
| **Left Alt** | Left Amiga key |
| **Right Alt** | Right Amiga key |
| **Escape** | Pause menu |

### Pause menu

Press **Escape** while in emulator mode to open the pause menu. From here you can:
- Adjust audio volume
- Enable / disable CRT effects
- Tune CRT parameters
- Remap gamepad buttons
- Return to the room (emulator keeps running in the background)

---

## Configuration

`Saved/RetroScreen.ini` is created on first run. The most useful options:

```ini
[RetroScreen]
LibretroCorePath=Plugins/UnrealLibretro/MyCores/puae_libretro.dll
LibretroRomPath=EmulatorData/MyGame.adf
RuntimeRegion=PAL          ; PAL (50 Hz) or NTSC (60 Hz)
EnableCrt=true
RuntimeAudioVolume=1.0

DefaultJoypadPort=0        ; 0 = joystick port 1 (most games), 1 = port 2
DefaultJoypadDeadzone=8192
DefaultJoypadMapAxesToDpad=true
DefaultJoypadInvertY=false

[RetroScreenCoreOptions]
; Optional PUAE core overrides — uncomment to use
; puae_model=a500
; puae_cpu_compatibility=normal
```

### Gamepad button remapping

Open the pause menu and choose **Remap Controls**. Remapping is saved automatically to `Saved/RetroScreen.ini` under `[RetroScreenButtonMap]`.

---

## CRT Display Settings

The cabinet screen applies a CRT shader by default. You can tune or disable it from the pause menu, or directly in `RetroScreen.ini`:

| Parameter | Range | Default | Effect |
|---|---|---|---|
| `CRT_Enabled` | 0 / 1 | 1 | Master on/off |
| `CRT_ScanlineIntensity` | 0.0 – 1.0 | 0.45 | Horizontal scanline darkness |
| `CRT_Curvature` | 0.0 – 1.0 | 0.18 | Screen barrel distortion |
| `CRT_PhosphorBloom` | 0.0 – 4.0 | 1.25 | Phosphor glow halo |
| `CRT_Vignette` | 0.0 – 1.0 | 0.30 | Corner darkening |
| `CRT_ChromaticAberration` | 0.0 – 2.0 | 0.25 | RGB fringing |

Set all to 0 / disable `CRT_Enabled` for a clean pixel-perfect display.

---

## Troubleshooting

### Setup guidance screen appears at launch
One or more required files are missing. Follow the on-screen instructions. The most common causes:

- **Kickstart ROM not found** — ensure your `.rom` file is in `EmulatorData/Kickstart/`
- **Game disk not found** — check `LibretroRomPath` in `Saved/RetroScreen.ini`
- **Core DLL not found** — place `puae_libretro.dll` in `Plugins/UnrealLibretro/MyCores/`

### Black screen on the cabinet
The emulator is running but no frame has been rendered yet. Wait a few seconds for the Amiga to boot. If it stays black, check the log at `Saved/Logs/RetroScreen.log` for core errors.

### No sound
Check `RuntimeAudioVolume` in `Saved/RetroScreen.ini`. Ensure your Windows audio device is active. Some Amiga games are silent during loading — wait for the game to start.

### Gamepad not responding
Ensure the gamepad is connected before launching. Check `DefaultJoypadPort` — most games use port 0 (joystick port 1). Some two-player games expect port 1.

### Poor performance / frame drops
- Verify your GPU supports DirectX 12 and Lumen (hardware ray tracing recommended)
- Lower shadow quality and Lumen settings in the pause menu if available
- Ensure no background applications are throttling CPU/GPU

### Wrong screen region / speed
Set `RuntimeRegion=PAL` for European games (50 Hz) or `RuntimeRegion=NTSC` for US/Japanese titles (60 Hz) in `Saved/RetroScreen.ini`.

---

## Legal

See [LEGAL.md](LEGAL.md) for the full legal notice.

**Summary:** RetroScreen is open-source software. It does not include any Amiga ROM or software. You are solely responsible for ensuring you have the legal right to use any ROM or disk image you provide.

---

*RetroScreen is developed by XTR – Xtended Realities. Built with Unreal Engine 5.*
