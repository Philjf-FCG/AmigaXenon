# Testing ArcadeCabinetActor - Quick Start

## What Was Created
- **ArcadeCabinetActor**: A display actor that shows the Amiga Xenon 2 game on a 3D arcade cabinet
- **RetroScreenManager Integration**: Auto-spawn cabinet on level start
- **Rusty Japanese Arcade Cabinet Model**: The definitive 3D cabinet mesh

## Testing in Editor

### Basic Setup (Fastest Test)
1. **Open AmigaXenon.uproject** in Unreal Engine 5.7
2. In Content Browser, search for and place **RetroScreenManager** in your level
3. In the Details panel, find the **RetroScreen|Cabinet** section
4. Set **bAutoSpawnArcadeCabinet = true**
5. Press **Play (PIE)**

**Expected Result**: You should see the Rusty Japanese arcade cabinet displayed fullscreen.

### What You Can Configure
In RetroScreenManager Details > "RetroScreen|Cabinet" section:

| Property | Default | Purpose |
|----------|---------|---------|
| **bAutoSpawnArcadeCabinet** | false | Enable auto-spawn on level start |
| **ArcadeCabinetActorClass** | (empty) | Leave empty to use default AArcadeCabinetActor |

### In ArcadeCabinetActor (if manually placed)
Details > "Arcade Cabinet" section:

| Property | Value | Purpose |
|----------|-------|---------|
| **ScreenMode** | Embedded | Display mode (Fullscreen hides cabinet, shows just screen) |
| **CameraDistance** | 80.0 | How far camera is from cabinet front |
| **CameraFieldOfView** | 75.0° | Camera zoom level |
| **ScreenHorizontalScale** | 0.95 | Screen width within cabinet |
| **ScreenVerticalScale** | 0.90 | Screen height within cabinet |

### Troubleshooting

**Issue**: Cabinet doesn't appear when Play is pressed
- **Fix**: Check RetroScreenManager is in the level and bAutoSpawnArcadeCabinet=true

**Issue**: Black screen on cabinet
- **Fix**: Emulator texture needs to be bound to ScreenMesh material (next phase)
- **Workaround**: This is expected - cabinet display system is built, texture binding comes next

**Issue**: "Cannot find cabinet mesh" error in Output Log
- **Fix**: Verify content exists: `Content/Fab/Rusty_Japanese_Arcade/rusty_japanese_arcade/StaticMeshes/rusty_japanese_arcade.uasset`

## Architecture Notes
- Cabinet hides all other actors (landscape, etc.) automatically
- Player controller camera is set to view camera on the cabinet
- Viewport aspect ratio dynamically scales cabinet view
- Pause menu will overlay on top of cabinet display

## Next Steps
1. Bind emulator framebuffer texture to ScreenMesh material
2. Integrate pause menu keyboard input
3. Test fullscreen vs embedded screen modes
4. Fine-tune camera distance/angles for arcade feel
