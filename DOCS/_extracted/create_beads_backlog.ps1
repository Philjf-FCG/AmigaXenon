Set-Location "c:\DevProjects\AmigaXenon"

function New-Issue {
    param(
        [string]$Title,
        [string]$Type,
        [int]$Priority,
        [string]$Labels,
        [string]$Description,
        [string]$Acceptance,
        [string]$Parent = ""
    )

    if ($Parent -ne "") {
        $id = & npx -y @beads/bd create --title $Title --type $Type --priority $Priority --labels $Labels --description $Description --acceptance $Acceptance --parent $Parent --silent
    }
    else {
        $id = & npx -y @beads/bd create --title $Title --type $Type --priority $Priority --labels $Labels --description $Description --acceptance $Acceptance --silent
    }

    if (-not $id) {
        throw "Failed to create issue: $Title"
    }
    return $id.Trim()
}

$common = "retroscreen,v1-plan,docs-derived"
$created = @()

$program = New-Issue -Title "RetroScreen v1.0 implementation program" -Type "epic" -Priority 1 -Labels "$common,program" -Description "Deliver the full RetroScreen v1.0 scope from overview, TDD, input, art, and implementation plan documents." -Acceptance "All planned phases are complete with acceptance criteria met and release package ready."
$created += [pscustomobject]@{ Phase = "Program"; Title = "RetroScreen v1.0 implementation program"; Id = $program }

$foundation = New-Issue -Title "Phase 0: Foundation and prerequisites" -Type "epic" -Priority 1 -Labels "$common,phase-0,foundation" -Description "Establish required toolchain, runtime assets, and baseline project configuration before emulator implementation." -Acceptance "Team can build UE C++ project and has legal test assets ready." -Parent $program
$created += [pscustomobject]@{ Phase = "Phase 0"; Title = "Phase 0: Foundation and prerequisites"; Id = $foundation }

$phase1 = New-Issue -Title "Phase 1: Emulator core proof of concept" -Type "epic" -Priority 1 -Labels "$common,phase-1,poc" -Description "Validate libretro Amiga core operation in a standalone harness outside Unreal Engine." -Acceptance "Standalone harness runs Amiga game with stable frame rate, audio, and keyboard input." -Parent $program
$created += [pscustomobject]@{ Phase = "Phase 1"; Title = "Phase 1: Emulator core proof of concept"; Id = $phase1 }

$phase2 = New-Issue -Title "Phase 2: Unreal Engine integration" -Type "epic" -Priority 1 -Labels "$common,phase-2,integration" -Description "Integrate emulator runtime into UE5 with threaded execution, texture output, and procedural audio." -Acceptance "In-game plane displays emulator output with synchronized low-latency audio." -Parent $program
$created += [pscustomobject]@{ Phase = "Phase 2"; Title = "Phase 2: Unreal Engine integration"; Id = $phase2 }

$phase3 = New-Issue -Title "Phase 3: Input and interaction flow" -Type "epic" -Priority 1 -Labels "$common,phase-3,input" -Description "Implement environment/emulator mode switching and full input passthrough mappings." -Acceptance "Player can enter and exit cabinet mode and play using keyboard, mouse, and gamepad." -Parent $program
$created += [pscustomobject]@{ Phase = "Phase 3"; Title = "Phase 3: Input and interaction flow"; Id = $phase3 }

$phase4 = New-Issue -Title "Phase 4: Art, environment, and CRT presentation" -Type "epic" -Priority 2 -Labels "$common,phase-4,art" -Description "Build final cabinet asset, arcade room, CRT shader, and environmental audiovisual immersion." -Acceptance "Scene quality and CRT presentation meet v1.0 art-direction goals." -Parent $program
$created += [pscustomobject]@{ Phase = "Phase 4"; Title = "Phase 4: Art, environment, and CRT presentation"; Id = $phase4 }

$phase5 = New-Issue -Title "Phase 5: Polish, configuration, and release prep" -Type "epic" -Priority 1 -Labels "$common,phase-5,release" -Description "Finalize UX, configuration surfaces, QA, optimization, and distribution packaging." -Acceptance "Non-technical user can configure and play with documented setup path." -Parent $program
$created += [pscustomobject]@{ Phase = "Phase 5"; Title = "Phase 5: Polish, configuration, and release prep"; Id = $phase5 }

$phase0Tasks = @(
    @{ T = "Install and verify Unreal Engine 5.4+ C++ toolchain"; D = "Prepare UE5.4+ and native build prerequisites for Windows and optional Linux target."; A = "Clean C++ project build succeeds from command line and editor." },
    @{ T = "Obtain and verify PUAE libretro core DLL"; D = "Acquire compatible PUAE core build and validate licensing constraints for project usage."; A = "Core binary loads in a test loader and license notes are recorded." },
    @{ T = "Prepare legal Kickstart ROM test asset"; D = "Provision user-supplied Kickstart ROM file for local testing only."; A = "Kickstart path is available and validated by startup checks." },
    @{ T = "Prepare baseline Amiga ADF test suite"; D = "Collect multiple ADFs for keyboard-only, mouse-only, and joystick-heavy validation."; A = "At least three representative ADFs are cataloged for QA." },
    @{ T = "Define architecture and coding conventions"; D = "Lock naming, threading, and module boundaries for manager and bridge classes before implementation."; A = "Team follows agreed conventions and module layout in source tree." }
)

$phase1Tasks = @(
    @{ T = "Create standalone C++ libretro harness"; D = "Set up a small executable that loads libretro core DLL at runtime."; A = "Harness starts and resolves required retro symbols." },
    @{ T = "Implement libretro environment callback"; D = "Implement retro_set_environment callback handling capabilities and pixel format negotiation."; A = "Core initializes successfully with expected environment responses." },
    @{ T = "Implement libretro video callback"; D = "Implement retro_set_video_refresh callback and frame capture path for display."; A = "Frames render consistently in test window." },
    @{ T = "Implement libretro audio callback"; D = "Implement retro_set_audio_sample_batch callback and output to local audio device."; A = "Audio plays without stutter during gameplay." },
    @{ T = "Implement libretro input callbacks"; D = "Implement input polling/state callbacks and keyboard forwarding."; A = "Game receives expected control input." },
    @{ T = "Load Kickstart and ADF into harness"; D = "Wire runtime loading paths for Kickstart ROM and game disk image."; A = "Selected test game boots successfully." },
    @{ T = "Capture timing and CPU metrics"; D = "Measure frame pacing and CPU cost under representative load."; A = "PAL target maintains stable 50fps on mid-range hardware." },
    @{ T = "Document Phase 1 benchmark results"; D = "Publish performance and compatibility findings to guide UE integration decisions."; A = "Benchmark report is shared and accepted." }
)

$phase2Tasks = @(
    @{ T = "Create RetroScreen UE5 C++ module scaffolding"; D = "Add module structure and build configuration for runtime emulator integration."; A = "Module compiles and loads in editor." },
    @{ T = "Implement ARetroScreenManager lifecycle actor"; D = "Create manager actor owning startup, shutdown, and high-level state orchestration."; A = "Manager can initialize and teardown emulator safely." },
    @{ T = "Run emulator core on FRunnableThread"; D = "Move emulator run loop off game thread with proper synchronization boundaries."; A = "Game thread remains responsive while emulator runs." },
    @{ T = "Implement double-buffered video bridge"; D = "Capture core frames into staging buffers and coordinate producer-consumer swapping."; A = "No tearing or race-condition artifacts in captured frames." },
    @{ T = "Upload frames to UTexture2DDynamic"; D = "Create dynamic texture pipeline and per-frame update path for emulator output."; A = "In-game texture updates at emulator cadence with low overhead." },
    @{ T = "Create baseline screen material and test plane"; D = "Apply dynamic texture to simple unlit material on a scene plane for verification."; A = "Playable game output appears in UE scene." },
    @{ T = "Implement procedural audio bridge with SPSC ring buffer"; D = "Feed emulator PCM into lock-free buffer consumed by USoundWaveProcedural or synth path."; A = "Audio remains synchronized and under latency target." },
    @{ T = "Profile integration performance and optimize hotspots"; D = "Measure texture upload, audio callback, and thread scheduling costs in UE."; A = "Integration meets initial latency and frame-rate targets." }
)

$phase3Tasks = @(
    @{ T = "Implement URetroScreenInputBridge core"; D = "Create bridge class managing shared atomic input states for emulator polling."; A = "Bridge exposes stable keyboard, mouse, and gamepad state to core." },
    @{ T = "Author Enhanced Input contexts for two modes"; D = "Create IMC_Environment and IMC_Emulator mapping contexts and action assets."; A = "Context swap cleanly changes active controls." },
    @{ T = "Build cabinet interaction and camera transition flow"; D = "Implement approach prompt, interact action, and camera lerp to cabinet anchor."; A = "Entering emulator mode feels smooth and deterministic." },
    @{ T = "Implement pause and return-to-room flow"; D = "Handle Escape pause menu and mode reversal while emulator keeps running."; A = "Exit path restores player control without stuck inputs." },
    @{ T = "Add mouse confinement and cursor policy"; D = "Hide/confine cursor in emulator mode and restore behavior in environment mode."; A = "Mouse cannot escape window during emulator interaction." },
    @{ T = "Implement gamepad mapping and analog-to-digital conversion"; D = "Map sticks/buttons to Amiga joystick keys with deadzone and diagonal support."; A = "Gamepad controls are responsive across tested games." },
    @{ T = "Implement runtime INI configuration reader"; D = "Load core path, ROM path, region, CRT toggle, volume, and input profile settings."; A = "Settings apply on startup without recompilation." },
    @{ T = "Validate input compatibility across game archetypes"; D = "Run regression passes against keyboard, mouse, and joystick-centric Amiga titles."; A = "No critical control regressions across test suite." }
)

$phase4Tasks = @(
    @{ T = "Create or source arcade cabinet hero asset"; D = "Model/adapt cabinet mesh with required dimensions and legal originality constraints."; A = "Cabinet asset is approved and import-ready." },
    @{ T = "Author cabinet UVs and material slots"; D = "Prepare slots for body, bezel, screen, controls, side art, coin door, and marquee."; A = "Material assignment supports runtime screen texture injection." },
    @{ T = "Implement CRT material core effects"; D = "Build scanlines, curvature, phosphor bloom, vignette, and chromatic aberration controls."; A = "CRT effect visually matches design intent." },
    @{ T = "Expose CRT parameters via dynamic material instance"; D = "Make runtime-adjustable parameters for quality and accessibility tuning."; A = "Parameters can be changed at runtime and in pause menu." },
    @{ T = "Build arcade room environment layout"; D = "Assemble room shell, props, signage, and spatial composition around hero cabinet."; A = "Environment meets target atmosphere and traversal needs." },
    @{ T = "Implement lighting with emissive CRT contribution"; D = "Tune Lumen setup and supplemental lights for believable cabinet glow."; A = "Screen illumination visibly affects nearby geometry." },
    @{ T = "Add environmental audio layers"; D = "Integrate room tone, hum, and ambient loops distinct from emulator audio source."; A = "Environment sounds cohesive and non-intrusive." },
    @{ T = "Add screen-linked supplementary rect light"; D = "Drive a cabinet light from average screen luminance/color for dynamic ambience."; A = "Light response tracks screen content without flicker artifacts." }
)

$phase5Tasks = @(
    @{ T = "Build pause and settings UMG interface"; D = "Implement resume, CRT settings, audio volume, input mapping, return, and quit controls."; A = "All planned controls are accessible and functional." },
    @{ T = "Implement first-launch setup and missing ROM guidance"; D = "Detect absent Kickstart/ROM paths and display legal setup instructions."; A = "First-run path clearly guides user to complete configuration." },
    @{ T = "Implement gamepad remapping user flow"; D = "Provide remapping UI and persistence for profile-specific controller bindings."; A = "Remaps persist and apply correctly in emulator mode." },
    @{ T = "Execute broad compatibility test matrix"; D = "Run test passes across varied Amiga titles and record compatibility outcomes."; A = "High-priority compatibility blockers are resolved or documented." },
    @{ T = "Run performance and memory optimization pass"; D = "Profile CPU, GPU, and memory; fix bottlenecks in rendering, audio, and threading."; A = "Performance targets are met on target hardware tier." },
    @{ T = "Write end-user README and legal disclaimer"; D = "Document setup, troubleshooting, controls, and legal requirements for user-supplied files."; A = "New user can complete setup without developer assistance." },
    @{ T = "Package distributable build with empty EmulatorData"; D = "Produce cooked build and verify no copyrighted ROM content is bundled."; A = "Distribution package is legally clean and functional." },
    @{ T = "Complete internal QA and release readiness review"; D = "Run smoke/regression checks and resolve launch blockers before release."; A = "Release candidate passes agreed quality gates." }
)

foreach ($i in $phase0Tasks) {
    $id = New-Issue -Title $i.T -Type "task" -Priority 2 -Labels "$common,phase-0" -Description $i.D -Acceptance $i.A -Parent $foundation
    $created += [pscustomobject]@{ Phase = "Phase 0"; Title = $i.T; Id = $id }
}
foreach ($i in $phase1Tasks) {
    $id = New-Issue -Title $i.T -Type "task" -Priority 1 -Labels "$common,phase-1" -Description $i.D -Acceptance $i.A -Parent $phase1
    $created += [pscustomobject]@{ Phase = "Phase 1"; Title = $i.T; Id = $id }
}
foreach ($i in $phase2Tasks) {
    $id = New-Issue -Title $i.T -Type "task" -Priority 1 -Labels "$common,phase-2" -Description $i.D -Acceptance $i.A -Parent $phase2
    $created += [pscustomobject]@{ Phase = "Phase 2"; Title = $i.T; Id = $id }
}
foreach ($i in $phase3Tasks) {
    $id = New-Issue -Title $i.T -Type "task" -Priority 1 -Labels "$common,phase-3" -Description $i.D -Acceptance $i.A -Parent $phase3
    $created += [pscustomobject]@{ Phase = "Phase 3"; Title = $i.T; Id = $id }
}
foreach ($i in $phase4Tasks) {
    $id = New-Issue -Title $i.T -Type "task" -Priority 2 -Labels "$common,phase-4" -Description $i.D -Acceptance $i.A -Parent $phase4
    $created += [pscustomobject]@{ Phase = "Phase 4"; Title = $i.T; Id = $id }
}
foreach ($i in $phase5Tasks) {
    $id = New-Issue -Title $i.T -Type "task" -Priority 1 -Labels "$common,phase-5" -Description $i.D -Acceptance $i.A -Parent $phase5
    $created += [pscustomobject]@{ Phase = "Phase 5"; Title = $i.T; Id = $id }
}

$csvPath = "DOCS\_extracted\beads_issue_map_2026-05-05.csv"
$created | Export-Csv -Path $csvPath -NoTypeInformation -Encoding UTF8

Write-Output "CREATED_TOTAL=$($created.Count)"
Write-Output "PROGRAM_ID=$program"
Write-Output "FOUNDATION_ID=$foundation"
Write-Output "PHASE1_ID=$phase1"
Write-Output "PHASE2_ID=$phase2"
Write-Output "PHASE3_ID=$phase3"
Write-Output "PHASE4_ID=$phase4"
Write-Output "PHASE5_ID=$phase5"
Write-Output "MAP_FILE=$csvPath"
