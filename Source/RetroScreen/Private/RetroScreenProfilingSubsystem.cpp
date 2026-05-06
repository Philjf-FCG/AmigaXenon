#include "RetroScreenProfilingSubsystem.h"

#include "RetroScreenManager.h"

#include "Async/Async.h"
#include "Engine/World.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool URetroScreenProfilingSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World != nullptr && World->IsGameWorld();
}

void URetroScreenProfilingSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    if (!FParse::Param(FCommandLine::Get(), TEXT("RetroScreenAutoProfile")))
    {
        return;
    }

    StartAutoProfiling(InWorld);
}

void URetroScreenProfilingSubsystem::Deinitialize()
{
    if (ActiveManager.IsValid())
    {
        ActiveManager->ShutdownEmulator();
        ActiveManager = nullptr;
    }

    bAutoProfilingActive = false;
    Super::Deinitialize();
}

void URetroScreenProfilingSubsystem::StartAutoProfiling(UWorld& InWorld)
{
    if (bAutoProfilingActive)
    {
        return;
    }

    bAutoProfilingActive = true;

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = TEXT("RetroScreenAutoProfileManager");
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    ARetroScreenManager* Manager = InWorld.SpawnActor<ARetroScreenManager>(ARetroScreenManager::StaticClass(), FTransform::Identity, SpawnParams);
    if (Manager == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RetroScreen auto profiling failed: manager spawn failed."));
        FPlatformMisc::RequestExit(false);
        return;
    }

    ActiveManager = Manager;

    Manager->SetRuntimeMetricsLoggingEnabled(true);
    Manager->SetRuntimeMetricsCsvExportEnabled(true);
    Manager->SetRuntimeQualityGateLoggingEnabled(true);

    if (!Manager->InitializeEmulator())
    {
        UE_LOG(LogTemp, Error, TEXT("RetroScreen auto profiling failed: manager initialization failed."));
        FPlatformMisc::RequestExit(false);
        return;
    }

    float ProfileSeconds = 15.0f;
    FParse::Value(FCommandLine::Get(), TEXT("RetroScreenProfileSeconds="), ProfileSeconds);
    ProfileSeconds = FMath::Clamp(ProfileSeconds, 2.0f, 300.0f);

    // Emit an initial snapshot so the profiler summary has a baseline row.
    Manager->ExportRuntimeMetricsCsv(TEXT("Saved/RetroScreenMetrics.csv"), false);

    UE_LOG(LogTemp, Log, TEXT("RetroScreen auto profiling active for %.2f seconds."), ProfileSeconds);

    TWeakObjectPtr<URetroScreenProfilingSubsystem> WeakThis(this);
    Async(EAsyncExecution::Thread, [WeakThis, ProfileSeconds]()
    {
        FPlatformProcess::Sleep(ProfileSeconds);

        AsyncTask(ENamedThreads::GameThread, [WeakThis]()
        {
            if (WeakThis.IsValid())
            {
                WeakThis->FinishAutoProfiling();
            }
        });
    });
}

void URetroScreenProfilingSubsystem::FinishAutoProfiling()
{
    if (ActiveManager.IsValid())
    {
        ActiveManager->LogRuntimeMetricsSnapshot();
        ActiveManager->ExportRuntimeMetricsCsv(TEXT("Saved/RetroScreenMetrics.csv"), true);
        ActiveManager->ShutdownEmulator();
        ActiveManager = nullptr;
    }

    bAutoProfilingActive = false;
    UE_LOG(LogTemp, Log, TEXT("RetroScreen auto profiling complete. Requesting exit."));
    FPlatformMisc::RequestExit(false);
}
