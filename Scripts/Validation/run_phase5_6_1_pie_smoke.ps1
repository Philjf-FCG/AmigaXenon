param(
    [string]$ProjectPath = "C:\DevProjects\AmigaXenon\AmigaXenon.uproject",
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.7"
)

$editorExe = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor.exe"

if (-not (Test-Path $ProjectPath)) {
    Write-Error "Project file not found: $ProjectPath"
    exit 1
}

if (-not (Test-Path $editorExe)) {
    Write-Error "UnrealEditor executable not found: $editorExe"
    exit 1
}

Write-Host "Launching Unreal Editor for Phase 5.1 PIE smoke test..."
Write-Host "Checklist: DOCS/RetroScreen_Phase5_6_1_PIE_SmokeChecklist_2026-05-06.md"

# Non-blocking launch so tester can run the checklist manually in editor.
Start-Process -FilePath $editorExe -ArgumentList @(
    $ProjectPath,
    "-NoSplash"
)
