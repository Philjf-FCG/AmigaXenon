#include "RetroScreenCompatibilityRunner.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

ARetroScreenCompatibilityRunner::ARetroScreenCompatibilityRunner()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ARetroScreenCompatibilityRunner::BeginPlay()
{
    Super::BeginPlay();
    ResolveManagerIfNeeded();
}

void ARetroScreenCompatibilityRunner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CancelTestMatrix();
    Super::EndPlay(EndPlayReason);
}

void ARetroScreenCompatibilityRunner::BeginTestMatrix()
{
    if (State != ERunnerState::Idle)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CompatibilityRunner] Already running — call CancelTestMatrix first."));
        return;
    }

    ResolveManagerIfNeeded();
    if (RetroScreenManager == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("[CompatibilityRunner] No RetroScreenManager found. Aborting matrix."));
        return;
    }

    Results.Reset();
    CurrentEntryIndex = -1;
    State = ERunnerState::Idle;

    UE_LOG(LogTemp, Display, TEXT("[CompatibilityRunner] Starting compatibility matrix (%d entries)."), TestMatrix.Num());
    AdvanceToNextEntry();
}

void ARetroScreenCompatibilityRunner::CancelTestMatrix()
{
    if (State == ERunnerState::Idle)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World != nullptr)
    {
        World->GetTimerManager().ClearTimer(WarmupTimerHandle);
        World->GetTimerManager().ClearTimer(TestDurationTimerHandle);
    }

    State = ERunnerState::Idle;
    UE_LOG(LogTemp, Warning, TEXT("[CompatibilityRunner] Test matrix cancelled at entry %d."), CurrentEntryIndex);
    OnMatrixComplete.Broadcast(Results);
}

void ARetroScreenCompatibilityRunner::AdvanceToNextEntry()
{
    ++CurrentEntryIndex;

    // Skip entries flagged as skipped
    while (CurrentEntryIndex < TestMatrix.Num() && TestMatrix[CurrentEntryIndex].bSkip)
    {
        FRetroScreenCompatibilityTestResult SkipResult;
        SkipResult.Entry = TestMatrix[CurrentEntryIndex];
        SkipResult.Result = ERetroScreenCompatibilityResult::Skipped;
        Results.Add(SkipResult);
        OnEntryComplete.Broadcast(SkipResult);
        UE_LOG(LogTemp, Display, TEXT("[CompatibilityRunner] Skipped: %s"), *TestMatrix[CurrentEntryIndex].GameName);
        ++CurrentEntryIndex;
    }

    if (CurrentEntryIndex >= TestMatrix.Num())
    {
        FinishMatrix();
        return;
    }

    StartCurrentEntry();
}

void ARetroScreenCompatibilityRunner::StartCurrentEntry()
{
    const FRetroScreenCompatibilityEntry& Entry = TestMatrix[CurrentEntryIndex];
    UE_LOG(LogTemp, Display, TEXT("[CompatibilityRunner] [%d/%d] Starting: %s"),
        CurrentEntryIndex + 1, TestMatrix.Num(), *Entry.GameName);

    if (RetroScreenManager->IsEmulatorRunning())
    {
        RetroScreenManager->ShutdownEmulator();
    }

    // Point the manager at the new ROM
    RetroScreenManager->SetLibretroRomPath(Entry.RomRelativePath);

    if (bResetMetricsPerEntry)
    {
        RetroScreenManager->ResetRuntimeMetrics();
    }

    State = ERunnerState::InitializingEmulator;
    const bool bLoaded = RetroScreenManager->InitializeEmulator();

    if (!bLoaded)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CompatibilityRunner] InitializeEmulator() failed for: %s"), *Entry.GameName);
        CaptureCurrentResult(false);
        return;
    }

    // Short warmup before we start measuring
    UWorld* World = GetWorld();
    if (World != nullptr)
    {
        World->GetTimerManager().SetTimer(
            WarmupTimerHandle,
            this,
            &ARetroScreenCompatibilityRunner::OnEmulatorWarmupComplete,
            EmulatorWarmupSeconds,
            false
        );
    }
}

void ARetroScreenCompatibilityRunner::OnEmulatorWarmupComplete()
{
    if (State != ERunnerState::InitializingEmulator)
    {
        return;
    }

    State = ERunnerState::RunningEmulator;

    if (bResetMetricsPerEntry)
    {
        RetroScreenManager->ResetRuntimeMetrics();
    }

    const float Duration = TestMatrix[CurrentEntryIndex].TestDurationSeconds;
    UWorld* World = GetWorld();
    if (World != nullptr)
    {
        World->GetTimerManager().SetTimer(
            TestDurationTimerHandle,
            this,
            &ARetroScreenCompatibilityRunner::OnTestDurationComplete,
            Duration,
            false
        );
    }
}

void ARetroScreenCompatibilityRunner::OnTestDurationComplete()
{
    if (State != ERunnerState::RunningEmulator)
    {
        return;
    }

    State = ERunnerState::CapturingResult;
    CaptureCurrentResult(true);
}

void ARetroScreenCompatibilityRunner::CaptureCurrentResult(bool bDidLoad)
{
    const FRetroScreenCompatibilityEntry& Entry = TestMatrix[CurrentEntryIndex];

    FRetroScreenCompatibilityTestResult TestResult;
    TestResult.Entry = Entry;

    if (!bDidLoad)
    {
        TestResult.Result = ERetroScreenCompatibilityResult::FailDidNotLoad;
        TestResult.FailureDetails = TEXT("InitializeEmulator() returned false");
    }
    else
    {
        TestResult.QualityGateResult = RetroScreenManager->EvaluateRuntimeQualityGate(Entry.QualityGate);
        if (TestResult.QualityGateResult.bPassed)
        {
            TestResult.Result = ERetroScreenCompatibilityResult::Pass;
        }
        else
        {
            TestResult.Result = ERetroScreenCompatibilityResult::FailQuality;
            TestResult.FailureDetails = TestResult.QualityGateResult.Summary;
        }
    }

    const FString Status = (TestResult.Result == ERetroScreenCompatibilityResult::Pass) ? TEXT("PASS") : TEXT("FAIL");
    UE_LOG(LogTemp, Display, TEXT("[CompatibilityRunner] [%d/%d] %s — %s: %s"),
        CurrentEntryIndex + 1, TestMatrix.Num(), *Status, *Entry.GameName, *TestResult.FailureDetails);

    Results.Add(TestResult);
    OnEntryComplete.Broadcast(TestResult);

    if (RetroScreenManager->IsEmulatorRunning())
    {
        RetroScreenManager->ShutdownEmulator();
    }

    State = ERunnerState::Idle;
    AdvanceToNextEntry();
}

void ARetroScreenCompatibilityRunner::FinishMatrix()
{
    State = ERunnerState::Complete;

    int32 PassCount = 0;
    int32 FailCount = 0;
    int32 SkipCount = 0;
    for (const FRetroScreenCompatibilityTestResult& R : Results)
    {
        if (R.Result == ERetroScreenCompatibilityResult::Pass) ++PassCount;
        else if (R.Result == ERetroScreenCompatibilityResult::Skipped) ++SkipCount;
        else ++FailCount;
    }

    UE_LOG(LogTemp, Display, TEXT("[CompatibilityRunner] Matrix complete. Pass: %d  Fail: %d  Skip: %d"),
        PassCount, FailCount, SkipCount);

    if (!AutoReportPath.IsEmpty())
    {
        const FString FullPath = FPaths::Combine(FPaths::ProjectSavedDir(), AutoReportPath);
        ExportReportCsv(FullPath);
    }

    OnMatrixComplete.Broadcast(Results);
    State = ERunnerState::Idle;
}

bool ARetroScreenCompatibilityRunner::ExportReportCsv(const FString& OutputPath) const
{
    if (Results.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CompatibilityRunner] No results to export."));
        return false;
    }

    IFileManager::Get().MakeDirectory(*FPaths::GetPath(OutputPath), true);

    FString Csv;
    Csv += TEXT("GameName,RomPath,Result,FailureDetails,TextureUploads,AvgUploadMs,MaxUploadMs,AudioUnderruns,AudioOverruns\n");

    for (const FRetroScreenCompatibilityTestResult& R : Results)
    {
        const FRetroScreenRuntimeMetrics& M = R.QualityGateResult.MetricsSnapshot;
        FString ResultStr;
        switch (R.Result)
        {
            case ERetroScreenCompatibilityResult::Pass:          ResultStr = TEXT("Pass");          break;
            case ERetroScreenCompatibilityResult::FailDidNotLoad: ResultStr = TEXT("FailLoad");     break;
            case ERetroScreenCompatibilityResult::FailQuality:   ResultStr = TEXT("FailQuality");   break;
            case ERetroScreenCompatibilityResult::Skipped:       ResultStr = TEXT("Skipped");       break;
            default:                                             ResultStr = TEXT("Unknown");       break;
        }

        // Escape any commas in free-text fields
        FString SafeName = R.Entry.GameName.Replace(TEXT(","), TEXT(";"));
        FString SafeDetails = R.FailureDetails.Replace(TEXT(","), TEXT(";"));

        Csv += FString::Printf(
            TEXT("%s,%s,%s,%s,%lld,%.2f,%.2f,%lld,%lld\n"),
            *SafeName,
            *R.Entry.RomRelativePath,
            *ResultStr,
            *SafeDetails,
            M.TextureUploadsSucceeded,
            M.AverageTextureUploadMs,
            M.MaxTextureUploadMs,
            M.AudioUnderrunSamples,
            M.AudioOverrunSamples
        );
    }

    const bool bOk = FFileHelper::SaveStringToFile(Csv, *OutputPath);
    if (bOk)
    {
        UE_LOG(LogTemp, Display, TEXT("[CompatibilityRunner] Report written to: %s"), *OutputPath);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[CompatibilityRunner] Failed to write report to: %s"), *OutputPath);
    }
    return bOk;
}

void ARetroScreenCompatibilityRunner::ResolveManagerIfNeeded()
{
    if (RetroScreenManager != nullptr)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    for (TActorIterator<ARetroScreenManager> It(World); It; ++It)
    {
        RetroScreenManager = *It;
        break;
    }
}
