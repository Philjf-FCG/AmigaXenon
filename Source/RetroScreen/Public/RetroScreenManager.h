#pragma once

#include <string>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RetroScreenAudioBridge.h"
#include "RetroScreenCabinetActor.h"
#include "RetroScreenEmulatorWorker.h"
#include "RetroScreenInputBridge.h"
#include "RetroScreenLibretroCore.h"
#include "RetroScreenTextureBridge.h"
#include "RetroScreenVideoBridge.h"

#include "RetroScreenSetupWidget.h"
#include "RetroScreenManager.generated.h"

class FRunnableThread;
class UTexture2D;
class UTextureRenderTarget2D;
class ULibretroCoreInstance;
class UAudioComponent;
class USoundWave;
class USceneComponent;
class UCameraComponent;
class UInputAction;
class UInputComponent;
class UInputMappingContext;
class URetroScreenPauseMenuWidget;
class URetroScreenSetupWidget;
class AArcadeCabinetActor;
struct FRetroScreenSetupIssues;
struct FInputActionValue;
struct FTimerHandle;

USTRUCT(BlueprintType)
struct FRetroScreenRuntimeMetrics
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int64 VideoFramesPublished = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int64 VideoFramesConsumed = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int64 LastVideoFrameSequence = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int64 TextureUploadsSucceeded = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int64 TextureUploadsFailed = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    float LastTextureUploadMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    float AverageTextureUploadMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    float MaxTextureUploadMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int32 AudioBufferedSamples = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int64 AudioUnderrunSamples = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int64 AudioOverrunSamples = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int64 AudioSamplesPushed = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int64 AudioSamplesPopped = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int64 InputPollCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    int64 WorkerFramesExecuted = 0;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    float WorkerLastFrameMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    float WorkerAverageFrameMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    float WorkerMaxFrameMs = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics")
    float LastMetricsUpdateSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct FRetroScreenQualityGateConfig
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|Metrics|QualityGate", meta = (ClampMin = "0"))
    int64 MinTextureUploadsForEvaluation = 30;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|Metrics|QualityGate", meta = (ClampMin = "0.0"))
    float MaxAverageTextureUploadMs = 4.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|Metrics|QualityGate", meta = (ClampMin = "0.0"))
    float MaxPeakTextureUploadMs = 12.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|Metrics|QualityGate", meta = (ClampMin = "0"))
    int64 MaxTextureUploadFailures = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|Metrics|QualityGate", meta = (ClampMin = "0"))
    int64 MaxAudioUnderrunSamples = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|Metrics|QualityGate", meta = (ClampMin = "0"))
    int64 MaxAudioOverrunSamples = 0;
};

USTRUCT(BlueprintType)
struct FRetroScreenQualityGateResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics|QualityGate")
    bool bPassed = false;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics|QualityGate")
    bool bEnoughTextureSamples = false;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics|QualityGate")
    bool bAverageUploadWithinThreshold = false;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics|QualityGate")
    bool bPeakUploadWithinThreshold = false;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics|QualityGate")
    bool bUploadFailuresWithinThreshold = false;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics|QualityGate")
    bool bAudioUnderrunWithinThreshold = false;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics|QualityGate")
    bool bAudioOverrunWithinThreshold = false;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics|QualityGate")
    FRetroScreenRuntimeMetrics MetricsSnapshot;

    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Metrics|QualityGate")
    FString Summary;
};

UENUM(BlueprintType)
enum class ERetroScreenInteractionInputMode : uint8
{
    Emulator UMETA(DisplayName = "Emulator"),
    Environment UMETA(DisplayName = "Environment")
};

USTRUCT(BlueprintType)
struct FRetroScreenPauseMenuSettings
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|PauseMenu", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float AudioVolume = 1.0f;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|PauseMenu")
    bool bEnableCrt = true;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|PauseMenu")
    FRetroScreenCrtParameters CrtParameters;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|PauseMenu", meta = (ClampMin = "0"))
    int32 JoypadPort = 0;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|PauseMenu", meta = (ClampMin = "0", ClampMax = "32767"))
    int32 JoypadDeadzone = 8192;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|PauseMenu")
    bool bMapAxesToDpad = true;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|PauseMenu")
    bool bInvertY = false;
};

UCLASS(BlueprintType, Blueprintable)
class RETROSCREEN_API ARetroScreenManager : public AActor
{
    GENERATED_BODY()

public:
    ARetroScreenManager();
    virtual ~ARetroScreenManager() override;

    UFUNCTION(BlueprintCallable, Category = "RetroScreen")
    bool InitializeEmulator();

    UFUNCTION(BlueprintCallable, Category = "RetroScreen")
    void ShutdownEmulator();

    UFUNCTION(BlueprintPure, Category = "RetroScreen")
    bool IsEmulatorRunning() const { return bEmulatorRunning; }

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Video")
    bool UploadLatestVideoFrame();

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Video")
    void PublishTestPatternFrame(int32 Width = 320, int32 Height = 256);

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Video")
    UTexture2D* GetEmulatorTexture() const { return EmulatorTexture; }

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Video")
    UTextureRenderTarget2D* GetLibretroRenderTarget() const { return LibretroRenderTarget; }

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Audio")
    void PublishTestToneAudio(float FrequencyHz = 440.0f, float DurationSeconds = 0.1f);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Input")
    void SetEmulatorDigitalInput(int32 Port, int32 Device, int32 Index, int32 Id, bool bPressed);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Input")
    void SetEmulatorAnalogInput(int32 Port, int32 Device, int32 Index, int32 Id, int32 Value);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Input")
    void SetEmulatorJoypadButton(int32 Port, int32 ButtonId, bool bPressed);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Input")
    void SetEmulatorJoypadAxes(int32 Port, int32 LeftX, int32 LeftY, int32 Deadzone = 8192, bool bMapAxesToDpad = true);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Input")
    void SetEmulatorJoypadAxesNormalized(int32 Port, float LeftXNormalized, float LeftYNormalized);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Input")
    void SetPrimaryEmulatorJoypadButton(int32 ButtonId, bool bPressed);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Input")
    void SetPrimaryEmulatorJoypadAxesNormalized(float LeftXNormalized, float LeftYNormalized);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen")
    void SetEmulatorPaused(bool bPaused);

    UFUNCTION(BlueprintPure, Category = "RetroScreen")
    bool IsEmulatorPaused() const { return bEmulatorPaused; }

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Input")
    void ReturnToEnvironmentMode(bool bPauseEmulator = true);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Input")
    void ResumeEmulatorInteraction(bool bUnpauseEmulator = true);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Input")
    void EnterCabinetInteraction(float BlendTimeOverride = -1.0f, bool bUnpauseEmulator = true);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Input")
    void SetInteractionInputMode(ERetroScreenInteractionInputMode NewMode);

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Input")
    ERetroScreenInteractionInputMode GetInteractionInputMode() const { return InteractionInputMode; }

    /**
     * Inspect the current config and return which required files are absent.
     * Called automatically on BeginPlay; also callable from Blueprint.
     */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Setup")
    FRetroScreenSetupIssues CheckSetupStatus() const;

    /** Show the setup guidance widget if any required file is absent. */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Setup")
    void ShowSetupWidgetIfNeeded();

    /** Dismiss the setup widget programmatically. */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Setup")
    void DismissSetupWidget();

    /** Widget class to use for the setup guidance overlay. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Setup")
    TSubclassOf<URetroScreenSetupWidget> SetupWidgetClass;

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|PauseMenu")
    void OpenPauseMenu();

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|PauseMenu")
    void ClosePauseMenu(bool bResumeEmulatorInteraction = true);

    UFUNCTION(BlueprintPure, Category = "RetroScreen|PauseMenu")
    bool IsPauseMenuOpen() const { return bPauseMenuOpen; }

    UFUNCTION(BlueprintPure, Category = "RetroScreen|PauseMenu")
    FRetroScreenPauseMenuSettings GetPauseMenuSettings() const;

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|PauseMenu")
    void ApplyPauseMenuSettings(const FRetroScreenPauseMenuSettings& NewSettings, bool bSaveToDisk = true);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Runtime")
    bool SaveRuntimeConfig();

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Audio")
    int32 GetBufferedAudioSampleCount() const;

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Metrics")
    FRetroScreenRuntimeMetrics GetRuntimeMetrics() const;

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Metrics")
    void ResetRuntimeMetrics();

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Metrics")
    void SetRuntimeMetricsLoggingEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Metrics")
    void LogRuntimeMetricsSnapshot();

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Metrics")
    bool ExportRuntimeMetricsCsv(const FString& CsvPath, bool bAppend = true) const;

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Metrics|QualityGate")
    FRetroScreenQualityGateResult EvaluateRuntimeQualityGate(const FRetroScreenQualityGateConfig& Config) const;

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Metrics|QualityGate")
    void LogRuntimeQualityGate(const FRetroScreenQualityGateConfig& Config);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Metrics|QualityGate")
    void SetRuntimeQualityGateLoggingEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Metrics")
    void SetRuntimeMetricsCsvExportEnabled(bool bEnabled);

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|Interaction")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|Interaction")
    TObjectPtr<UCameraComponent> CabinetCamera;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Config")
    FString RetroScreenConfigPath;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Libretro")
    bool bUseUnrealLibretroCore;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Libretro")
    FString LibretroCorePath;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Libretro")
    FString LibretroRomPath;

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|State")
    bool bEmulatorRunning;

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|State")
    bool bEmulatorPaused;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Input")
    ERetroScreenInteractionInputMode InteractionInputMode;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Input", meta = (ClampMin = "0"))
    int32 DefaultJoypadPort;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Input", meta = (ClampMin = "0", ClampMax = "32767"))
    int32 DefaultJoypadDeadzone;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Input")
    bool bDefaultJoypadMapAxesToDpad;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Input")
    bool bDefaultJoypadInvertY;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Input|Enhanced", meta = (ClampMin = "0"))
    int32 EnvironmentInputPriority;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Input|Enhanced", meta = (ClampMin = "0"))
    int32 EmulatorInputPriority;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Input|Enhanced")
    TObjectPtr<UInputMappingContext> EnvironmentInputMappingContext;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Input|Enhanced")
    TObjectPtr<UInputMappingContext> EmulatorInputMappingContext;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Input|Enhanced")
    bool bUseDefaultEnhancedInputMappings;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InputActionEnterCabinet;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InputActionPauseOrExit;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InputActionPrimaryFire;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InputActionMoveUp;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InputActionMoveDown;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InputActionMoveLeft;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InputActionMoveRight;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InputActionMoveAxisX;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> InputActionMoveAxisY;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Interaction")
    bool bEnableCabinetCameraTransition;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Interaction", meta = (ClampMin = "0.0"))
    float CabinetEnterBlendTime;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Interaction", meta = (ClampMin = "0.0"))
    float CabinetExitBlendTime;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Runtime")
    FString RuntimeRegion;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Runtime")
    bool bRuntimeCrtEnabled;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Runtime")
    FRetroScreenCrtParameters RuntimeCrtParameters;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Runtime", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float RuntimeAudioVolume;

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|PauseMenu")
    bool bPauseMenuOpen;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|PauseMenu")
    TSubclassOf<URetroScreenPauseMenuWidget> PauseMenuWidgetClass;

    UPROPERTY(Transient)
    TObjectPtr<URetroScreenPauseMenuWidget> PauseMenuWidgetInstance;

    UPROPERTY(Transient)
    TObjectPtr<URetroScreenSetupWidget> SetupWidgetInstance;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet")
    TSubclassOf<AArcadeCabinetActor> ArcadeCabinetActorClass;

    UPROPERTY(Transient)
    TObjectPtr<AArcadeCabinetActor> ArcadeCabinetActorInstance;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet")
    bool bAutoSpawnArcadeCabinet;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Metrics")
    bool bLogRuntimeMetricsToOutput;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Metrics", meta = (ClampMin = "0.1"))
    float RuntimeMetricsLogIntervalSeconds;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Metrics")
    bool bExportRuntimeMetricsCsv;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Metrics", meta = (ClampMin = "0.1"))
    float RuntimeMetricsCsvExportIntervalSeconds;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Metrics")
    FString RuntimeMetricsCsvPath;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Video")
    bool bAutoUploadVideoFrame;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Audio")
    bool bDrainStandaloneAudioInTick;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Audio", meta = (ClampMin = "0"))
    int32 StandaloneAudioDrainSamplesPerTick;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Video", meta = (ClampMin = "1.0"))
    float MaxVideoUploadsPerSecond;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Metrics|QualityGate")
    bool bLogRuntimeQualityGateToOutput;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Metrics|QualityGate", meta = (ClampMin = "0.1"))
    float RuntimeQualityGateLogIntervalSeconds;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Metrics|QualityGate")
    FRetroScreenQualityGateConfig RuntimeQualityGateConfig;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> EmulatorTexture;

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> LibretroRenderTarget;

    UPROPERTY(Transient)
    TObjectPtr<ULibretroCoreInstance> LibretroCoreInstance;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> LibretroAudioComponent;

    TUniquePtr<FRetroScreenEmulatorWorker> EmulatorWorker;
    TUniquePtr<FRetroScreenVideoBridge> VideoBridge;
    TUniquePtr<FRetroScreenTextureBridge> TextureBridge;
    TUniquePtr<FRetroScreenAudioBridge> AudioBridge;
    TUniquePtr<FRetroScreenInputBridge> InputBridge;
    TUniquePtr<FRetroScreenLibretroCore> LibretroCoreLoader;
    FRunnableThread* EmulatorThread;

    mutable FCriticalSection MetricsMutex;
    FRetroScreenRuntimeMetrics RuntimeMetrics;
    double TextureUploadAccumulatedMs;
    uint64 SyntheticFrameCounter;
    double SyntheticAudioPhaseRadians;
    double LastVideoUploadSeconds;
    TArray<uint8> WorkerScratchPixels;
    TArray<float> WorkerScratchAudio;
    TArray<int16> WorkerScratchAudioInt16;
    TMap<FString, std::string> RuntimeCoreOptionsAnsi;
    std::string RuntimeSystemDirectoryAnsi;
    std::string RuntimeSaveDirectoryAnsi;
    FThreadSafeBool bEmulatorInputEnabled;
    FTimerHandle RuntimeMetricsLogTimerHandle;
    FTimerHandle RuntimeMetricsCsvTimerHandle;
    FTimerHandle RuntimeQualityGateLogTimerHandle;
    TWeakObjectPtr<UInputComponent> BoundInputComponent;
    TWeakObjectPtr<AActor> CachedEnvironmentViewTarget;
    bool bEnhancedInputActionsBound;
    float CachedJoypadAxisX;
    float CachedJoypadAxisY;

    void HandleLibretroInputPoll();
    int16 HandleLibretroInputState(uint32 Port, uint32 Device, uint32 Index, uint32 Id) const;
    bool HandleLibretroEnvironmentCallback(uint32 Command, void* Data);
    void HandleLibretroVideoRefresh(const uint8* Data, int32 Width, int32 Height, int32 Pitch);
    int32 HandleLibretroAudioSampleBatch(const int16* Samples, int32 Frames);
    void StepEmulatorFrameOnWorkerThread();
    void LoadRuntimeConfig();
    void UpdateAudioMetrics();
    void UpdateWorkerMetrics();
    void StartRuntimeMetricsLogging();
    void StopRuntimeMetricsLogging();
    void StartRuntimeMetricsCsvExport();
    void StopRuntimeMetricsCsvExport();
    void ExportRuntimeMetricsCsvTick();
    void StartRuntimeQualityGateLogging();
    void StopRuntimeQualityGateLogging();
    void LogRuntimeQualityGateTick();
    void InitializeDefaultInputMappings();
    void ApplyInputMappingContext();
    void BindInputActions();
    void ApplyCabinetViewTransition(bool bToCabinet, float BlendTimeOverride = -1.0f);
    void ApplyInteractionInputMode();
    void HandleActionEnterCabinet(const FInputActionValue& Value);
    void HandleActionPauseOrExit(const FInputActionValue& Value);
    void HandleActionPrimaryFireStarted(const FInputActionValue& Value);
    void HandleActionPrimaryFireCompleted(const FInputActionValue& Value);
    void HandleActionMoveUpStarted(const FInputActionValue& Value);
    void HandleActionMoveUpCompleted(const FInputActionValue& Value);
    void HandleActionMoveDownStarted(const FInputActionValue& Value);
    void HandleActionMoveDownCompleted(const FInputActionValue& Value);
    void HandleActionMoveLeftStarted(const FInputActionValue& Value);
    void HandleActionMoveLeftCompleted(const FInputActionValue& Value);
    void HandleActionMoveRightStarted(const FInputActionValue& Value);
    void HandleActionMoveRightCompleted(const FInputActionValue& Value);
    void HandleActionMoveAxisX(const FInputActionValue& Value);
    void HandleActionMoveAxisY(const FInputActionValue& Value);
    void ShowPauseMenuWidget();
    void HidePauseMenuWidget();
    void ShowSetupWidget(const FRetroScreenSetupIssues& Issues);

    /**
     * Find or spawn arcade cabinet actor in the current world
     */
    void FindOrSpawnArcadeCabinet();

    /**
     * Get the arcade cabinet actor instance (nullptr if not spawned)
     */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Cabinet")
    AArcadeCabinetActor* GetArcadeCabinet() const { return ArcadeCabinetActorInstance; }

    void SetRuntimeCoreOption(const FString& Key, const FString& Value);
    bool InitializeUnrealLibretroCore();
    void ShutdownUnrealLibretroCore();

    UFUNCTION()
    void HandleLibretroLaunchComplete(const UTextureRenderTarget2D* LibretroFramebuffer, const USoundWave* AudioBuffer, bool bSuccess);

    UFUNCTION()
    void HandleLibretroCoreFramebufferResize();
};
