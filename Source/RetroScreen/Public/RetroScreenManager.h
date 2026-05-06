#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "RetroScreenManager.generated.h"

class FRunnableThread;
class FRetroScreenEmulatorWorker;
class FRetroScreenVideoBridge;
class FRetroScreenTextureBridge;
class FRetroScreenAudioBridge;
class UTexture2D;

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
    float LastMetricsUpdateSeconds = 0.0f;
};

UCLASS(BlueprintType, Blueprintable)
class RETROSCREEN_API ARetroScreenManager : public AActor
{
    GENERATED_BODY()

public:
    ARetroScreenManager();

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

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Audio")
    void PublishTestToneAudio(float FrequencyHz = 440.0f, float DurationSeconds = 0.1f);

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Audio")
    int32 GetBufferedAudioSampleCount() const;

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Metrics")
    FRetroScreenRuntimeMetrics GetRuntimeMetrics() const;

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Metrics")
    void ResetRuntimeMetrics();

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(EditAnywhere, Category = "RetroScreen|Config")
    FString RetroScreenConfigPath;

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|State")
    bool bEmulatorRunning;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> EmulatorTexture;

    TUniquePtr<FRetroScreenEmulatorWorker> EmulatorWorker;
    TUniquePtr<FRetroScreenVideoBridge> VideoBridge;
    TUniquePtr<FRetroScreenTextureBridge> TextureBridge;
    TUniquePtr<FRetroScreenAudioBridge> AudioBridge;
    FRunnableThread* EmulatorThread;

    mutable FCriticalSection MetricsMutex;
    FRetroScreenRuntimeMetrics RuntimeMetrics;
    double TextureUploadAccumulatedMs;

    void UpdateAudioMetrics();
};
