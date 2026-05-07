#include "RetroScreenGameMode.h"

#include "ArcadeCabinetActor.h"
#include "ArcadeRoomActor.h"
#include "RetroScreenManager.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/DefaultPawn.h"
#include "GameFramework/PlayerStart.h"

ARetroScreenGameMode::ARetroScreenGameMode()
{
    DefaultPawnClass = ADefaultPawn::StaticClass();
}

void ARetroScreenGameMode::StartPlay()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        Super::StartPlay();
        return;
    }

    // Spawn a PlayerStart behind the cabinet so the player's initial pawn
    // is in a sensible place before the manager steals the view target.
    bool bHasPlayerStart = false;
    for (TActorIterator<APlayerStart> It(World); It; ++It) { bHasPlayerStart = true; break; }
    if (!bHasPlayerStart)
    {
        FActorSpawnParameters PS;
        PS.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        World->SpawnActor<APlayerStart>(FVector(-400.0f, 0.0f, 100.0f), FRotator(0.0f, 0.0f, 0.0f), PS);
    }

    // Arcade room environment shell (floor, walls, ceiling, ambient light).
    bool bHasRoom = false;
    for (TActorIterator<AArcadeRoomActor> It(World); It; ++It) { bHasRoom = true; break; }
    if (!bHasRoom)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        World->SpawnActor<AArcadeRoomActor>(FTransform::Identity, Params);
    }

    // Emulator orchestrator.  FindOrSpawnArcadeCabinet() runs in its BeginPlay
    // (bAutoSpawnArcadeCabinet = true) and will set the player controller's view
    // target to the cabinet camera.  Do NOT pre-spawn the cabinet here — let the
    // manager do it so the camera handoff code in FindOrSpawnArcadeCabinet runs.
    bool bHasManager = false;
    for (TActorIterator<ARetroScreenManager> It(World); It; ++It) { bHasManager = true; break; }
    if (!bHasManager)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        World->SpawnActor<ARetroScreenManager>(FTransform::Identity, Params);
    }

    // Trigger BeginPlay on all actors (including the ones we just spawned).
    Super::StartPlay();
}
