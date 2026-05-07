#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RetroScreenManager.h"

#include "RetroScreenCompatibilityRunner.generated.h"

UENUM(BlueprintType)
enum class ERetroScreenCompatibilityResult : uint8
{
    Unknown       UMETA(DisplayName = "Unknown"),
    Pass          UMETA(DisplayName = "Pass"),
    FailDidNotLoad UMETA(DisplayName = "Fail - Did Not Load"),
    FailQuality   UMETA(DisplayName = "Fail - Quality Gate"),
    Skipped       UMETA(DisplayName = "Skipped"),
};

USTRUCT(BlueprintType)
struct FRetroScreenCompatibilityEntry
{
    GENERATED_BODY()

    /** Human-readable game title used in the report. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
    FString GameName;

    /** ROM or ADF path relative to the project's EmulatorData directory. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
    FString RomRelativePath;

    /** Seconds to run the emulator before evaluating the quality gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility", meta = (ClampMin = "2.0", ClampMax = "120.0"))
    float TestDurationSeconds = 10.0f;

    /** Whether this game is expected to load successfully; unexpected load failure is a blocker. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
    bool bExpectedToLoad = true;

    /** Skip this entry without running it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
    bool bSkip = false;

    /** Quality gate thresholds to pass for this title. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compatibility")
    FRetroScreenQualityGateConfig QualityGate;
};

USTRUCT(BlueprintType)
struct FRetroScreenCompatibilityTestResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Compatibility")
    FRetroScreenCompatibilityEntry Entry;

    UPROPERTY(BlueprintReadOnly, Category = "Compatibility")
    ERetroScreenCompatibilityResult Result = ERetroScreenCompatibilityResult::Unknown;

    UPROPERTY(BlueprintReadOnly, Category = "Compatibility")
    FString FailureDetails;

    UPROPERTY(BlueprintReadOnly, Category = "Compatibility")
    FRetroScreenQualityGateResult QualityGateResult;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCompatibilityEntryComplete,
    const FRetroScreenCompatibilityTestResult&, EntryResult);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCompatibilityMatrixComplete,
    const TArray<FRetroScreenCompatibilityTestResult>&, AllResults);

UCLASS(BlueprintType, Blueprintable)
class RETROSCREEN_API ARetroScreenCompatibilityRunner : public AActor
{
    GENERATED_BODY()

public:
    ARetroScreenCompatibilityRunner();

    /** Start running the full test matrix from the beginning. */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Compatibility")
    void BeginTestMatrix();

    /** Abort the running test matrix. */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Compatibility")
    void CancelTestMatrix();

    /** Export results accumulated so far to a CSV file. Returns false if nothing has run yet. */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Compatibility")
    bool ExportReportCsv(const FString& OutputPath) const;

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Compatibility")
    bool IsRunning() const { return State != ERunnerState::Idle; }

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Compatibility")
    int32 GetCurrentEntryIndex() const { return CurrentEntryIndex; }

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Compatibility")
    TArray<FRetroScreenCompatibilityTestResult> GetResults() const { return Results; }

    /** Called when each individual entry finishes. */
    UPROPERTY(BlueprintAssignable, Category = "RetroScreen|Compatibility")
    FOnCompatibilityEntryComplete OnEntryComplete;

    /** Called when the full matrix finishes or is cancelled. */
    UPROPERTY(BlueprintAssignable, Category = "RetroScreen|Compatibility")
    FOnCompatibilityMatrixComplete OnMatrixComplete;

    /** Test entries to run. Populated in the Details panel. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Compatibility")
    TArray<FRetroScreenCompatibilityEntry> TestMatrix;

    /** Manager to drive. Auto-found if null. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Compatibility")
    TObjectPtr<ARetroScreenManager> RetroScreenManager;

    /**
     * Path (relative to project Saved dir) where the CSV report is automatically
     * written when the full matrix completes.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Compatibility")
    FString AutoReportPath = TEXT("RetroScreen/CompatibilityReport.csv");

    /** Reset the emulator metrics at the start of each test so each entry is measured independently. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Compatibility")
    bool bResetMetricsPerEntry = true;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    enum class ERunnerState : uint8
    {
        Idle,
        InitializingEmulator,
        RunningEmulator,
        CapturingResult,
        Complete,
    };

    void AdvanceToNextEntry();
    void StartCurrentEntry();
    void OnEmulatorWarmupComplete();
    void OnTestDurationComplete();
    void CaptureCurrentResult(bool bDidLoad);
    void FinishMatrix();
    void ResolveManagerIfNeeded();

    ERunnerState State = ERunnerState::Idle;
    int32 CurrentEntryIndex = -1;
    TArray<FRetroScreenCompatibilityTestResult> Results;

    FTimerHandle WarmupTimerHandle;
    FTimerHandle TestDurationTimerHandle;

    /** Seconds to wait after InitializeEmulator() before starting metric capture. */
    static constexpr float EmulatorWarmupSeconds = 2.0f;
};
