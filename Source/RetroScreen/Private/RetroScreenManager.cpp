#include "RetroScreenManager.h"

#include "HAL/PlatformTime.h"
#include "HAL/RunnableThread.h"
#include "RetroScreenAudioBridge.h"
#include "RetroScreenEmulatorWorker.h"
#include "RetroScreenTextureBridge.h"
#include "RetroScreenVideoBridge.h"

ARetroScreenManager::ARetroScreenManager()
{
    PrimaryActorTick.bCanEverTick = false;

    bEmulatorRunning = false;
    EmulatorTexture = nullptr;
    RetroScreenConfigPath = TEXT("Saved/RetroScreen.ini");
    EmulatorThread = nullptr;
    TextureUploadAccumulatedMs = 0.0;
}

void ARetroScreenManager::BeginPlay()
{
    Super::BeginPlay();
}

bool ARetroScreenManager::InitializeEmulator()
{
    if (bEmulatorRunning)
    {
        return true;
    }

    // Placeholder for libretro core load + callback registration.
    VideoBridge = MakeUnique<FRetroScreenVideoBridge>();
    TextureBridge = MakeUnique<FRetroScreenTextureBridge>();
    AudioBridge = MakeUnique<FRetroScreenAudioBridge>();
    AudioBridge->Initialize(48000 * 2); // ~500ms stereo buffer at 48kHz
    ResetRuntimeMetrics();
    UpdateAudioMetrics();

    EmulatorWorker = MakeUnique<FRetroScreenEmulatorWorker>();
    EmulatorThread = FRunnableThread::Create(EmulatorWorker.Get(), TEXT("RetroScreenEmulatorThread"));
    if (EmulatorThread == nullptr)
    {
        EmulatorWorker.Reset();
        VideoBridge.Reset();
        TextureBridge.Reset();
        AudioBridge.Reset();
        return false;
    }

    bEmulatorRunning = true;
    return bEmulatorRunning;
}

void ARetroScreenManager::ShutdownEmulator()
{
    if (!bEmulatorRunning)
    {
        return;
    }

    // Placeholder for core teardown and buffer cleanup.
    if (EmulatorWorker.IsValid())
    {
        EmulatorWorker->Stop();
    }

    if (EmulatorThread != nullptr)
    {
        EmulatorThread->WaitForCompletion();
        delete EmulatorThread;
        EmulatorThread = nullptr;
    }

    EmulatorWorker.Reset();
    if (VideoBridge.IsValid())
    {
        VideoBridge->Reset();
    }
    VideoBridge.Reset();
    TextureBridge.Reset();
    AudioBridge.Reset();
    EmulatorTexture = nullptr;
    ResetRuntimeMetrics();
    bEmulatorRunning = false;
}

void ARetroScreenManager::PublishTestToneAudio(float FrequencyHz, float DurationSeconds)
{
    if (!AudioBridge.IsValid())
    {
        return;
    }

    const int32 SampleRate = 48000;
    const int32 Channels = 2;
    const int32 NumFrames = FMath::Max(1, FMath::RoundToInt(DurationSeconds * static_cast<float>(SampleRate)));

    TArray<float> Samples;
    Samples.SetNumUninitialized(NumFrames * Channels);

    for (int32 FrameIndex = 0; FrameIndex < NumFrames; ++FrameIndex)
    {
        const float T = static_cast<float>(FrameIndex) / static_cast<float>(SampleRate);
        const float Value = 0.2f * FMath::Sin(2.0f * PI * FrequencyHz * T);

        const int32 Offset = FrameIndex * Channels;
        Samples[Offset + 0] = Value;
        Samples[Offset + 1] = Value;
    }

    AudioBridge->PushInterleavedSamples(Samples.GetData(), Samples.Num());
    UpdateAudioMetrics();
}

int32 ARetroScreenManager::GetBufferedAudioSampleCount() const
{
    if (!AudioBridge.IsValid())
    {
        return 0;
    }

    return AudioBridge->GetAvailableSamples();
}

FRetroScreenRuntimeMetrics ARetroScreenManager::GetRuntimeMetrics() const
{
    FScopeLock Lock(&MetricsMutex);
    return RuntimeMetrics;
}

void ARetroScreenManager::ResetRuntimeMetrics()
{
    FScopeLock Lock(&MetricsMutex);
    RuntimeMetrics = FRetroScreenRuntimeMetrics();
    TextureUploadAccumulatedMs = 0.0;
}

bool ARetroScreenManager::UploadLatestVideoFrame()
{
    if (!VideoBridge.IsValid() || !TextureBridge.IsValid())
    {
        return false;
    }

    FRetroScreenVideoFrame LatestFrame;
    if (!VideoBridge->ConsumeLatestFrame(LatestFrame))
    {
        return false;
    }

    {
        FScopeLock Lock(&MetricsMutex);
        RuntimeMetrics.VideoFramesConsumed += 1;
        RuntimeMetrics.LastVideoFrameSequence = static_cast<int64>(LatestFrame.Sequence);
    }

    EmulatorTexture = TextureBridge->EnsureTexture(this, EmulatorTexture, LatestFrame.Width, LatestFrame.Height);
    if (EmulatorTexture == nullptr)
    {
        FScopeLock Lock(&MetricsMutex);
        RuntimeMetrics.TextureUploadsFailed += 1;
        RuntimeMetrics.LastMetricsUpdateSeconds = static_cast<float>(FPlatformTime::Seconds());
        return false;
    }

    const double StartSeconds = FPlatformTime::Seconds();
    const bool bUploaded = TextureBridge->UploadFrame(EmulatorTexture, LatestFrame);
    const float UploadMs = static_cast<float>((FPlatformTime::Seconds() - StartSeconds) * 1000.0);

    {
        FScopeLock Lock(&MetricsMutex);
        RuntimeMetrics.LastTextureUploadMs = UploadMs;
        RuntimeMetrics.MaxTextureUploadMs = FMath::Max(RuntimeMetrics.MaxTextureUploadMs, UploadMs);

        if (bUploaded)
        {
            RuntimeMetrics.TextureUploadsSucceeded += 1;
            TextureUploadAccumulatedMs += UploadMs;
            RuntimeMetrics.AverageTextureUploadMs = static_cast<float>(TextureUploadAccumulatedMs / RuntimeMetrics.TextureUploadsSucceeded);
        }
        else
        {
            RuntimeMetrics.TextureUploadsFailed += 1;
        }

        RuntimeMetrics.LastMetricsUpdateSeconds = static_cast<float>(FPlatformTime::Seconds());
    }

    UpdateAudioMetrics();
    return bUploaded;
}

void ARetroScreenManager::PublishTestPatternFrame(int32 Width, int32 Height)
{
    if (!VideoBridge.IsValid() || Width <= 0 || Height <= 0)
    {
        return;
    }

    const int32 Pitch = Width * 4;
    TArray<uint8> Pixels;
    Pixels.SetNumZeroed(Pitch * Height);

    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            const int32 PixelOffset = (Y * Pitch) + (X * 4);
            Pixels[PixelOffset + 0] = static_cast<uint8>((X * 255) / Width);     // B
            Pixels[PixelOffset + 1] = static_cast<uint8>((Y * 255) / Height);    // G
            Pixels[PixelOffset + 2] = static_cast<uint8>(96);                     // R
            Pixels[PixelOffset + 3] = 255;                                        // A
        }
    }

    const uint64 Sequence = VideoBridge->PublishFrame(Pixels.GetData(), Width, Height, Pitch);
    if (Sequence > 0)
    {
        FScopeLock Lock(&MetricsMutex);
        RuntimeMetrics.VideoFramesPublished += 1;
        RuntimeMetrics.LastVideoFrameSequence = static_cast<int64>(Sequence);
        RuntimeMetrics.LastMetricsUpdateSeconds = static_cast<float>(FPlatformTime::Seconds());
    }
}

void ARetroScreenManager::UpdateAudioMetrics()
{
    if (!AudioBridge.IsValid())
    {
        return;
    }

    FScopeLock Lock(&MetricsMutex);
    RuntimeMetrics.AudioBufferedSamples = AudioBridge->GetAvailableSamples();
    RuntimeMetrics.AudioUnderrunSamples = static_cast<int64>(AudioBridge->GetUnderrunCount());
    RuntimeMetrics.AudioOverrunSamples = static_cast<int64>(AudioBridge->GetOverrunCount());
    RuntimeMetrics.AudioSamplesPushed = static_cast<int64>(AudioBridge->GetTotalSamplesPushed());
    RuntimeMetrics.AudioSamplesPopped = static_cast<int64>(AudioBridge->GetTotalSamplesPopped());
    RuntimeMetrics.LastMetricsUpdateSeconds = static_cast<float>(FPlatformTime::Seconds());
}
