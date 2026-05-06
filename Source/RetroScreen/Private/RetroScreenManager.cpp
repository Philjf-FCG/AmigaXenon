#include "RetroScreenManager.h"

#include <cstdarg>
#include <cstdio>

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/RunnableThread.h"
#include "LibretroCoreInstance.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealLibretro.h"
#include "libretro/libretro.h"
#include "Components/AudioComponent.h"
#include "RetroScreenAudioBridge.h"
#include "RetroScreenEmulatorWorker.h"
#include "RetroScreenInputBridge.h"
#include "RetroScreenLibretroCore.h"
#include "RetroScreenTextureBridge.h"
#include "RetroScreenVideoBridge.h"
#include "TimerManager.h"

namespace
{
FString ExtractCoreOptionDefaultValue(const FString& Description)
{
    int32 SemicolonIndex = INDEX_NONE;
    if (!Description.FindChar(TEXT(';'), SemicolonIndex))
    {
        return FString();
    }

    FString Choices = Description.Mid(SemicolonIndex + 1).TrimStartAndEnd();

    int32 PipeIndex = INDEX_NONE;
    if (Choices.FindChar(TEXT('|'), PipeIndex))
    {
        Choices = Choices.Left(PipeIndex);
    }

    return Choices.TrimStartAndEnd();
}

void RETRO_CALLCONV RetroScreenCoreLogCallback(enum retro_log_level Level, const char* Format, ...)
{
    if (Format == nullptr)
    {
        return;
    }

    char MessageBuffer[2048];
    MessageBuffer[0] = '\0';

    va_list Args;
    va_start(Args, Format);
    std::vsnprintf(MessageBuffer, sizeof(MessageBuffer), Format, Args);
    va_end(Args);

    const TCHAR* Message = UTF8_TO_TCHAR(MessageBuffer);
    switch (Level)
    {
        case RETRO_LOG_ERROR:
            UE_LOG(LogTemp, Error, TEXT("RetroScreen core: %s"), Message);
            break;
        case RETRO_LOG_WARN:
            UE_LOG(LogTemp, Warning, TEXT("RetroScreen core: %s"), Message);
            break;
        case RETRO_LOG_INFO:
            UE_LOG(LogTemp, Log, TEXT("RetroScreen core: %s"), Message);
            break;
        case RETRO_LOG_DEBUG:
        default:
            UE_LOG(LogTemp, Verbose, TEXT("RetroScreen core: %s"), Message);
            break;
    }
}
}

ARetroScreenManager::ARetroScreenManager()
{
    PrimaryActorTick.bCanEverTick = true;

    bEmulatorRunning = false;
    bEmulatorPaused = false;
    InteractionInputMode = ERetroScreenInteractionInputMode::Emulator;
    DefaultJoypadPort = 0;
    DefaultJoypadDeadzone = 8192;
    bDefaultJoypadMapAxesToDpad = true;
    bDefaultJoypadInvertY = false;
    RuntimeRegion = TEXT("PAL");
    bRuntimeCrtEnabled = true;
    RuntimeAudioVolume = 1.0f;
    bUseUnrealLibretroCore = false;
    LibretroCorePath = TEXT("");
    LibretroRomPath = TEXT("");
    bLogRuntimeMetricsToOutput = true;
    RuntimeMetricsLogIntervalSeconds = 2.0f;
    bExportRuntimeMetricsCsv = false;
    RuntimeMetricsCsvExportIntervalSeconds = 2.0f;
    RuntimeMetricsCsvPath = TEXT("Saved/RetroScreenMetrics.csv");
    bAutoUploadVideoFrame = true;
    MaxVideoUploadsPerSecond = 60.0f;
    bLogRuntimeQualityGateToOutput = false;
    RuntimeQualityGateLogIntervalSeconds = 2.0f;
    EmulatorTexture = nullptr;
    LibretroRenderTarget = nullptr;
    LibretroCoreInstance = nullptr;
    LibretroAudioComponent = nullptr;
    RetroScreenConfigPath = TEXT("Saved/RetroScreen.ini");
    EmulatorThread = nullptr;
    TextureUploadAccumulatedMs = 0.0;
    SyntheticFrameCounter = 0;
    SyntheticAudioPhaseRadians = 0.0;
    LastVideoUploadSeconds = 0.0;
    bEmulatorInputEnabled = true;
}

ARetroScreenManager::~ARetroScreenManager() = default;

void ARetroScreenManager::BeginPlay()
{
    Super::BeginPlay();
    LoadRuntimeConfig();
    ApplyInteractionInputMode();
    StartRuntimeMetricsLogging();
    StartRuntimeMetricsCsvExport();
    StartRuntimeQualityGateLogging();
}

void ARetroScreenManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!bAutoUploadVideoFrame || !bEmulatorRunning || bUseUnrealLibretroCore)
    {
        return;
    }

    const double NowSeconds = FPlatformTime::Seconds();
    const double MinIntervalSeconds = 1.0 / FMath::Max(1.0, static_cast<double>(MaxVideoUploadsPerSecond));
    if ((NowSeconds - LastVideoUploadSeconds) < MinIntervalSeconds)
    {
        return;
    }

    if (UploadLatestVideoFrame())
    {
        LastVideoUploadSeconds = NowSeconds;
    }
}

void ARetroScreenManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopRuntimeMetricsLogging();
    StopRuntimeMetricsCsvExport();
    StopRuntimeQualityGateLogging();
    Super::EndPlay(EndPlayReason);
}

bool ARetroScreenManager::InitializeEmulator()
{
    if (bEmulatorRunning)
    {
        return true;
    }

    bEmulatorPaused = false;

    if (bUseUnrealLibretroCore)
    {
        return InitializeUnrealLibretroCore();
    }

    // Placeholder for libretro core load + callback registration.
    VideoBridge = MakeUnique<FRetroScreenVideoBridge>();
    TextureBridge = MakeUnique<FRetroScreenTextureBridge>();
    AudioBridge = MakeUnique<FRetroScreenAudioBridge>();
    InputBridge = MakeUnique<FRetroScreenInputBridge>();
    AudioBridge->Initialize(48000 * 2); // ~500ms stereo buffer at 48kHz
    ResetRuntimeMetrics();
    UpdateAudioMetrics();
    UpdateWorkerMetrics();
    SyntheticFrameCounter = 0;
    SyntheticAudioPhaseRadians = 0.0;
    WorkerScratchPixels.Reset();
    WorkerScratchAudio.Reset();
    WorkerScratchAudioInt16.Reset();
    RuntimeSystemDirectoryAnsi.clear();
    RuntimeSaveDirectoryAnsi.clear();

    {
        const FString SystemDirectory = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
        FString SaveDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RetroScreen"));
        IFileManager::Get().MakeDirectory(*SaveDirectory, true);
        SaveDirectory = FPaths::ConvertRelativePathToFull(SaveDirectory);

        const FTCHARToUTF8 SystemUtf8(*SystemDirectory);
        RuntimeSystemDirectoryAnsi.assign(SystemUtf8.Get(), static_cast<size_t>(SystemUtf8.Length()));

        const FTCHARToUTF8 SaveUtf8(*SaveDirectory);
        RuntimeSaveDirectoryAnsi.assign(SaveUtf8.Get(), static_cast<size_t>(SaveUtf8.Length()));
    }

    LibretroCoreLoader = MakeUnique<FRetroScreenLibretroCore>();
    if (!LibretroCorePath.TrimStartAndEnd().IsEmpty())
    {
        FRetroScreenLibretroCallbacks LibretroCallbacks;
        LibretroCallbacks.EnvironmentCallback = [this](uint32 Command, void* Data)
        {
            return HandleLibretroEnvironmentCallback(Command, Data);
        };
        LibretroCallbacks.VideoRefreshCallback = [this](const uint8* Data, int32 Width, int32 Height, int32 Pitch)
        {
            HandleLibretroVideoRefresh(Data, Width, Height, Pitch);
        };
        LibretroCallbacks.AudioSampleBatchCallback = [this](const int16* Samples, int32 Frames)
        {
            return HandleLibretroAudioSampleBatch(Samples, Frames);
        };
        LibretroCallbacks.InputPollCallback = [this]()
        {
            HandleLibretroInputPoll();
        };
        LibretroCallbacks.InputStateCallback = [this](uint32 Port, uint32 Device, uint32 Index, uint32 Id)
        {
            return HandleLibretroInputState(Port, Device, Index, Id);
        };

        FString CoreLoadError;
        if (!LibretroCoreLoader->LoadCore(LibretroCorePath, LibretroCallbacks, CoreLoadError))
        {
            UE_LOG(LogTemp, Warning, TEXT("RetroScreen standalone libretro core load failed: %s"), *CoreLoadError);
            LibretroCoreLoader.Reset();
            InputBridge.Reset();
            AudioBridge.Reset();
            TextureBridge.Reset();
            VideoBridge.Reset();
            return false;
        }

        FString GameLoadError;
        if (!LibretroCoreLoader->LoadGame(LibretroRomPath, GameLoadError))
        {
            UE_LOG(LogTemp, Warning, TEXT("RetroScreen standalone libretro game load failed: %s"), *GameLoadError);
            LibretroCoreLoader->Shutdown();
            LibretroCoreLoader.Reset();
            InputBridge.Reset();
            AudioBridge.Reset();
            TextureBridge.Reset();
            VideoBridge.Reset();
            return false;
        }
    }
    else
    {
        LibretroCoreLoader.Reset();
    }

    EmulatorWorker = MakeUnique<FRetroScreenEmulatorWorker>(50.0f);
    EmulatorWorker->SetPaused(false);
    EmulatorWorker->SetFrameStepCallback([this]()
    {
        StepEmulatorFrameOnWorkerThread();
    });
    EmulatorThread = FRunnableThread::Create(EmulatorWorker.Get(), TEXT("RetroScreenEmulatorThread"));
    if (EmulatorThread == nullptr)
    {
        EmulatorWorker.Reset();
        VideoBridge.Reset();
        TextureBridge.Reset();
        AudioBridge.Reset();
        InputBridge.Reset();
        if (LibretroCoreLoader.IsValid())
        {
            LibretroCoreLoader->Shutdown();
            LibretroCoreLoader.Reset();
        }
        return false;
    }

    bEmulatorRunning = true;
    StartRuntimeMetricsLogging();
    StartRuntimeMetricsCsvExport();
    StartRuntimeQualityGateLogging();
    return bEmulatorRunning;
}

void ARetroScreenManager::ShutdownEmulator()
{
    if (bUseUnrealLibretroCore)
    {
        ShutdownUnrealLibretroCore();
        return;
    }

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
    InputBridge.Reset();
    if (LibretroCoreLoader.IsValid())
    {
        LibretroCoreLoader->Shutdown();
        LibretroCoreLoader.Reset();
    }
    EmulatorTexture = nullptr;
    StopRuntimeMetricsLogging();
    StopRuntimeMetricsCsvExport();
    StopRuntimeQualityGateLogging();
    ResetRuntimeMetrics();
    bEmulatorPaused = false;
    bEmulatorRunning = false;
}

bool ARetroScreenManager::InitializeUnrealLibretroCore()
{
    if (LibretroCorePath.TrimStartAndEnd().IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("RetroScreen libretro initialization failed: CorePath is empty."));
        return false;
    }

    const FString ResolvedCorePath = FUnrealLibretroModule::ResolveCorePath(LibretroCorePath);
    if (!IFileManager::Get().FileExists(*ResolvedCorePath))
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("RetroScreen libretro initialization failed: core not found at resolved path: %s"),
            *ResolvedCorePath
        );
        return false;
    }

    if (!LibretroRomPath.TrimStartAndEnd().IsEmpty())
    {
        const FString ResolvedRomPath = FUnrealLibretroModule::ResolveROMPath(LibretroRomPath);
        const bool bRomExists =
            IFileManager::Get().FileExists(*ResolvedRomPath) ||
            IFileManager::Get().DirectoryExists(*ResolvedRomPath);

        if (!bRomExists)
        {
            UE_LOG(
                LogTemp,
                Warning,
                TEXT("RetroScreen libretro initialization failed: rom not found at resolved path: %s"),
                *ResolvedRomPath
            );
            return false;
        }
    }

    if (LibretroAudioComponent == nullptr)
    {
        LibretroAudioComponent = NewObject<UAudioComponent>(this, TEXT("RetroScreenLibretroAudio"));
        if (LibretroAudioComponent != nullptr)
        {
            LibretroAudioComponent->RegisterComponent();
        }
    }

    if (LibretroCoreInstance == nullptr)
    {
        LibretroCoreInstance = NewObject<ULibretroCoreInstance>(this, TEXT("RetroScreenLibretroCore"));
        if (LibretroCoreInstance == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("RetroScreen libretro initialization failed: unable to create core component."));
            return false;
        }

        LibretroCoreInstance->RegisterComponent();
    }

    if (LibretroAudioComponent == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("RetroScreen libretro initialization failed: unable to create audio component."));
        return false;
    }

    LibretroCoreInstance->OnLaunchComplete.RemoveDynamic(this, &ARetroScreenManager::HandleLibretroLaunchComplete);
    LibretroCoreInstance->OnCoreFrameBufferResize.RemoveDynamic(this, &ARetroScreenManager::HandleLibretroCoreFramebufferResize);
    LibretroCoreInstance->OnLaunchComplete.AddDynamic(this, &ARetroScreenManager::HandleLibretroLaunchComplete);
    LibretroCoreInstance->OnCoreFrameBufferResize.AddDynamic(this, &ARetroScreenManager::HandleLibretroCoreFramebufferResize);

    LibretroCoreInstance->AudioComponent = LibretroAudioComponent;
    LibretroAudioComponent->SetVolumeMultiplier(RuntimeAudioVolume);
    LibretroCoreInstance->CorePath = LibretroCorePath;
    LibretroCoreInstance->RomPath = LibretroRomPath;

    LibretroRenderTarget = nullptr;
    bEmulatorPaused = false;
    ResetRuntimeMetrics();
    UpdateAudioMetrics();

    LibretroCoreInstance->Launch();
    bEmulatorRunning = true;
    StartRuntimeMetricsLogging();
    StartRuntimeMetricsCsvExport();
    StartRuntimeQualityGateLogging();
    return true;
}

void ARetroScreenManager::ShutdownUnrealLibretroCore()
{
    if (LibretroCoreInstance != nullptr)
    {
        LibretroCoreInstance->OnLaunchComplete.RemoveDynamic(this, &ARetroScreenManager::HandleLibretroLaunchComplete);
        LibretroCoreInstance->OnCoreFrameBufferResize.RemoveDynamic(this, &ARetroScreenManager::HandleLibretroCoreFramebufferResize);
        LibretroCoreInstance->Shutdown();
    }

    if (LibretroAudioComponent != nullptr)
    {
        LibretroAudioComponent->Stop();
    }

    LibretroRenderTarget = nullptr;
    StopRuntimeMetricsLogging();
    StopRuntimeMetricsCsvExport();
    StopRuntimeQualityGateLogging();
    ResetRuntimeMetrics();
    bEmulatorPaused = false;
    bEmulatorRunning = false;
}

void ARetroScreenManager::HandleLibretroLaunchComplete(const UTextureRenderTarget2D* LibretroFramebuffer, const USoundWave* AudioBuffer, bool bSuccess)
{
    if (!bSuccess)
    {
        const FString ErrorMessage =
            (LibretroCoreInstance != nullptr) ? LibretroCoreInstance->LastErrorMessage : TEXT("unknown error");
        UE_LOG(LogTemp, Warning, TEXT("RetroScreen libretro launch failed: %s"), *ErrorMessage);
        LibretroRenderTarget = nullptr;
        bEmulatorRunning = false;
        return;
    }

    LibretroRenderTarget = const_cast<UTextureRenderTarget2D*>(LibretroFramebuffer);
    {
        FScopeLock Lock(&MetricsMutex);
        RuntimeMetrics.LastMetricsUpdateSeconds = static_cast<float>(FPlatformTime::Seconds());
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("RetroScreen libretro launch succeeded. Core=%s Rom=%s AudioBuffer=%s"),
        *LibretroCorePath,
        *LibretroRomPath,
        (AudioBuffer != nullptr) ? TEXT("present") : TEXT("null")
    );
}

void ARetroScreenManager::HandleLibretroCoreFramebufferResize()
{
    if (LibretroCoreInstance == nullptr)
    {
        return;
    }

    {
        FScopeLock Lock(&MetricsMutex);
        RuntimeMetrics.LastMetricsUpdateSeconds = static_cast<float>(FPlatformTime::Seconds());
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("RetroScreen libretro framebuffer resize: %d x %d"),
        LibretroCoreInstance->FrameWidth,
        LibretroCoreInstance->FrameHeight
    );
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
        const float Value = (0.2f * RuntimeAudioVolume) * FMath::Sin(2.0f * PI * FrequencyHz * T);

        const int32 Offset = FrameIndex * Channels;
        Samples[Offset + 0] = Value;
        Samples[Offset + 1] = Value;
    }

    AudioBridge->PushInterleavedSamples(Samples.GetData(), Samples.Num());
    UpdateAudioMetrics();
}

void ARetroScreenManager::SetEmulatorDigitalInput(int32 Port, int32 Device, int32 Index, int32 Id, bool bPressed)
{
    if (!InputBridge.IsValid())
    {
        return;
    }

    InputBridge->SetDigitalState(
        static_cast<uint32>(FMath::Max(0, Port)),
        static_cast<uint32>(FMath::Max(0, Device)),
        static_cast<uint32>(FMath::Max(0, Index)),
        static_cast<uint32>(FMath::Max(0, Id)),
        bPressed
    );
}

void ARetroScreenManager::SetEmulatorAnalogInput(int32 Port, int32 Device, int32 Index, int32 Id, int32 Value)
{
    if (!InputBridge.IsValid())
    {
        return;
    }

    InputBridge->SetAnalogState(
        static_cast<uint32>(FMath::Max(0, Port)),
        static_cast<uint32>(FMath::Max(0, Device)),
        static_cast<uint32>(FMath::Max(0, Index)),
        static_cast<uint32>(FMath::Max(0, Id)),
        static_cast<int16>(FMath::Clamp(Value, static_cast<int32>(MIN_int16), static_cast<int32>(MAX_int16)))
    );
}

void ARetroScreenManager::SetEmulatorJoypadButton(int32 Port, int32 ButtonId, bool bPressed)
{
    if (!InputBridge.IsValid())
    {
        return;
    }

    InputBridge->SetJoypadButton(
        static_cast<uint32>(FMath::Max(0, Port)),
        static_cast<uint32>(FMath::Max(0, ButtonId)),
        bPressed
    );
}

void ARetroScreenManager::SetEmulatorJoypadAxes(int32 Port, int32 LeftX, int32 LeftY, int32 Deadzone, bool bMapAxesToDpad)
{
    if (!InputBridge.IsValid())
    {
        return;
    }

    InputBridge->SetJoypadAxes(
        static_cast<uint32>(FMath::Max(0, Port)),
        static_cast<int16>(FMath::Clamp(LeftX, static_cast<int32>(MIN_int16), static_cast<int32>(MAX_int16))),
        static_cast<int16>(FMath::Clamp(LeftY, static_cast<int32>(MIN_int16), static_cast<int32>(MAX_int16))),
        static_cast<int16>(FMath::Clamp(Deadzone, 0, static_cast<int32>(MAX_int16))),
        bMapAxesToDpad
    );
}

void ARetroScreenManager::SetEmulatorJoypadAxesNormalized(int32 Port, float LeftXNormalized, float LeftYNormalized)
{
    const float ClampedX = FMath::Clamp(LeftXNormalized, -1.0f, 1.0f);
    const float ClampedY = FMath::Clamp(LeftYNormalized, -1.0f, 1.0f);
    const float EffectiveY = bDefaultJoypadInvertY ? -ClampedY : ClampedY;

    SetEmulatorJoypadAxes(
        Port,
        FMath::RoundToInt(ClampedX * static_cast<float>(MAX_int16)),
        FMath::RoundToInt(EffectiveY * static_cast<float>(MAX_int16)),
        DefaultJoypadDeadzone,
        bDefaultJoypadMapAxesToDpad
    );
}

void ARetroScreenManager::SetPrimaryEmulatorJoypadButton(int32 ButtonId, bool bPressed)
{
    SetEmulatorJoypadButton(DefaultJoypadPort, ButtonId, bPressed);
}

void ARetroScreenManager::SetPrimaryEmulatorJoypadAxesNormalized(float LeftXNormalized, float LeftYNormalized)
{
    SetEmulatorJoypadAxesNormalized(DefaultJoypadPort, LeftXNormalized, LeftYNormalized);
}

void ARetroScreenManager::SetEmulatorPaused(bool bPaused)
{
    if (bEmulatorPaused == bPaused)
    {
        return;
    }

    bEmulatorPaused = bPaused;

    if (EmulatorWorker.IsValid())
    {
        EmulatorWorker->SetPaused(bPaused);
    }

    if (InputBridge.IsValid())
    {
        InputBridge->ClearStates();
    }
}

void ARetroScreenManager::ReturnToEnvironmentMode(bool bPauseEmulator)
{
    if (bPauseEmulator)
    {
        SetEmulatorPaused(true);
    }

    SetInteractionInputMode(ERetroScreenInteractionInputMode::Environment);
}

void ARetroScreenManager::ResumeEmulatorInteraction(bool bUnpauseEmulator)
{
    SetInteractionInputMode(ERetroScreenInteractionInputMode::Emulator);

    if (bUnpauseEmulator)
    {
        SetEmulatorPaused(false);
    }
}

void ARetroScreenManager::SetInteractionInputMode(ERetroScreenInteractionInputMode NewMode)
{
    if (InteractionInputMode == NewMode)
    {
        return;
    }

    InteractionInputMode = NewMode;

    if (InputBridge.IsValid())
    {
        InputBridge->ClearStates();
    }

    ApplyInteractionInputMode();
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

void ARetroScreenManager::SetRuntimeMetricsLoggingEnabled(bool bEnabled)
{
    bLogRuntimeMetricsToOutput = bEnabled;
    if (bLogRuntimeMetricsToOutput)
    {
        StartRuntimeMetricsLogging();
        return;
    }

    StopRuntimeMetricsLogging();
}

void ARetroScreenManager::SetRuntimeMetricsCsvExportEnabled(bool bEnabled)
{
    bExportRuntimeMetricsCsv = bEnabled;
    if (bExportRuntimeMetricsCsv)
    {
        StartRuntimeMetricsCsvExport();
        return;
    }

    StopRuntimeMetricsCsvExport();
}

void ARetroScreenManager::SetRuntimeQualityGateLoggingEnabled(bool bEnabled)
{
    bLogRuntimeQualityGateToOutput = bEnabled;
    if (bLogRuntimeQualityGateToOutput)
    {
        StartRuntimeQualityGateLogging();
        return;
    }

    StopRuntimeQualityGateLogging();
}

void ARetroScreenManager::LogRuntimeMetricsSnapshot()
{
    UpdateAudioMetrics();
    UpdateWorkerMetrics();
    const FRetroScreenRuntimeMetrics Metrics = GetRuntimeMetrics();

    UE_LOG(
        LogTemp,
        Log,
        TEXT("RetroScreen Metrics | Frames pub=%lld con=%lld seq=%lld | Upload ok=%lld fail=%lld last=%.3fms avg=%.3fms max=%.3fms | Audio buf=%d push=%lld pop=%lld underrun=%lld overrun=%lld | Input poll=%lld | Worker frames=%lld last=%.3fms avg=%.3fms max=%.3fms"),
        static_cast<long long>(Metrics.VideoFramesPublished),
        static_cast<long long>(Metrics.VideoFramesConsumed),
        static_cast<long long>(Metrics.LastVideoFrameSequence),
        static_cast<long long>(Metrics.TextureUploadsSucceeded),
        static_cast<long long>(Metrics.TextureUploadsFailed),
        Metrics.LastTextureUploadMs,
        Metrics.AverageTextureUploadMs,
        Metrics.MaxTextureUploadMs,
        Metrics.AudioBufferedSamples,
        static_cast<long long>(Metrics.AudioSamplesPushed),
        static_cast<long long>(Metrics.AudioSamplesPopped),
        static_cast<long long>(Metrics.AudioUnderrunSamples),
        static_cast<long long>(Metrics.AudioOverrunSamples),
        static_cast<long long>(Metrics.InputPollCount),
        static_cast<long long>(Metrics.WorkerFramesExecuted),
        Metrics.WorkerLastFrameMs,
        Metrics.WorkerAverageFrameMs,
        Metrics.WorkerMaxFrameMs
    );
}

bool ARetroScreenManager::ExportRuntimeMetricsCsv(const FString& CsvPath, bool bAppend) const
{
    if (CsvPath.IsEmpty())
    {
        return false;
    }

    FString ResolvedPath = CsvPath;
    if (FPaths::IsRelative(ResolvedPath))
    {
        ResolvedPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), ResolvedPath);
    }

    IFileManager& FileManager = IFileManager::Get();
    FileManager.MakeDirectory(*FPaths::GetPath(ResolvedPath), true);

    const bool bFileExists = FileManager.FileExists(*ResolvedPath);
    const bool bWriteHeader = !bAppend || !bFileExists;

    const FRetroScreenRuntimeMetrics Metrics = GetRuntimeMetrics();

    FString CsvText;
    if (bWriteHeader)
    {
        CsvText += TEXT("timestamp_utc,video_frames_published,video_frames_consumed,last_video_frame_sequence,texture_uploads_succeeded,texture_uploads_failed,last_texture_upload_ms,avg_texture_upload_ms,max_texture_upload_ms,audio_buffered_samples,audio_underrun_samples,audio_overrun_samples,audio_samples_pushed,audio_samples_popped,input_poll_count,worker_frames_executed,worker_last_frame_ms,worker_avg_frame_ms,worker_max_frame_ms,last_metrics_update_seconds\n");
    }

    const FString TimestampUtc = FDateTime::UtcNow().ToString(TEXT("%Y-%m-%dT%H:%M:%SZ"));
    CsvText += FString::Printf(
        TEXT("%s,%lld,%lld,%lld,%lld,%lld,%.6f,%.6f,%.6f,%d,%lld,%lld,%lld,%lld,%lld,%lld,%.6f,%.6f,%.6f,%.6f\n"),
        *TimestampUtc,
        static_cast<long long>(Metrics.VideoFramesPublished),
        static_cast<long long>(Metrics.VideoFramesConsumed),
        static_cast<long long>(Metrics.LastVideoFrameSequence),
        static_cast<long long>(Metrics.TextureUploadsSucceeded),
        static_cast<long long>(Metrics.TextureUploadsFailed),
        Metrics.LastTextureUploadMs,
        Metrics.AverageTextureUploadMs,
        Metrics.MaxTextureUploadMs,
        Metrics.AudioBufferedSamples,
        static_cast<long long>(Metrics.AudioUnderrunSamples),
        static_cast<long long>(Metrics.AudioOverrunSamples),
        static_cast<long long>(Metrics.AudioSamplesPushed),
        static_cast<long long>(Metrics.AudioSamplesPopped),
        static_cast<long long>(Metrics.InputPollCount),
        static_cast<long long>(Metrics.WorkerFramesExecuted),
        Metrics.WorkerLastFrameMs,
        Metrics.WorkerAverageFrameMs,
        Metrics.WorkerMaxFrameMs,
        Metrics.LastMetricsUpdateSeconds
    );

    const uint32 WriteFlags = bAppend ? FILEWRITE_Append : 0;
    return FFileHelper::SaveStringToFile(
        CsvText,
        *ResolvedPath,
        FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
        &FileManager,
        WriteFlags
    );
}

FRetroScreenQualityGateResult ARetroScreenManager::EvaluateRuntimeQualityGate(const FRetroScreenQualityGateConfig& Config) const
{
    FRetroScreenQualityGateResult Result;
    Result.MetricsSnapshot = GetRuntimeMetrics();

    const int64 RequiredUploads = FMath::Max<int64>(0, Config.MinTextureUploadsForEvaluation);
    Result.bEnoughTextureSamples = Result.MetricsSnapshot.TextureUploadsSucceeded >= RequiredUploads;

    Result.bAverageUploadWithinThreshold =
        Result.bEnoughTextureSamples &&
        (Result.MetricsSnapshot.AverageTextureUploadMs <= Config.MaxAverageTextureUploadMs);

    Result.bPeakUploadWithinThreshold =
        Result.bEnoughTextureSamples &&
        (Result.MetricsSnapshot.MaxTextureUploadMs <= Config.MaxPeakTextureUploadMs);

    Result.bUploadFailuresWithinThreshold =
        Result.MetricsSnapshot.TextureUploadsFailed <= Config.MaxTextureUploadFailures;

    Result.bAudioUnderrunWithinThreshold =
        Result.MetricsSnapshot.AudioUnderrunSamples <= Config.MaxAudioUnderrunSamples;

    Result.bAudioOverrunWithinThreshold =
        Result.MetricsSnapshot.AudioOverrunSamples <= Config.MaxAudioOverrunSamples;

    Result.bPassed =
        Result.bEnoughTextureSamples &&
        Result.bAverageUploadWithinThreshold &&
        Result.bPeakUploadWithinThreshold &&
        Result.bUploadFailuresWithinThreshold &&
        Result.bAudioUnderrunWithinThreshold &&
        Result.bAudioOverrunWithinThreshold;

    TArray<FString> FailureReasons;
    if (!Result.bEnoughTextureSamples)
    {
        FailureReasons.Add(FString::Printf(
            TEXT("insufficient texture samples: got=%lld required=%lld"),
            static_cast<long long>(Result.MetricsSnapshot.TextureUploadsSucceeded),
            static_cast<long long>(RequiredUploads)
        ));
    }

    if (!Result.bAverageUploadWithinThreshold)
    {
        FailureReasons.Add(FString::Printf(
            TEXT("avg upload ms too high: value=%.3f threshold=%.3f"),
            Result.MetricsSnapshot.AverageTextureUploadMs,
            Config.MaxAverageTextureUploadMs
        ));
    }

    if (!Result.bPeakUploadWithinThreshold)
    {
        FailureReasons.Add(FString::Printf(
            TEXT("peak upload ms too high: value=%.3f threshold=%.3f"),
            Result.MetricsSnapshot.MaxTextureUploadMs,
            Config.MaxPeakTextureUploadMs
        ));
    }

    if (!Result.bUploadFailuresWithinThreshold)
    {
        FailureReasons.Add(FString::Printf(
            TEXT("texture upload failures above threshold: value=%lld threshold=%lld"),
            static_cast<long long>(Result.MetricsSnapshot.TextureUploadsFailed),
            static_cast<long long>(Config.MaxTextureUploadFailures)
        ));
    }

    if (!Result.bAudioUnderrunWithinThreshold)
    {
        FailureReasons.Add(FString::Printf(
            TEXT("audio underrun samples above threshold: value=%lld threshold=%lld"),
            static_cast<long long>(Result.MetricsSnapshot.AudioUnderrunSamples),
            static_cast<long long>(Config.MaxAudioUnderrunSamples)
        ));
    }

    if (!Result.bAudioOverrunWithinThreshold)
    {
        FailureReasons.Add(FString::Printf(
            TEXT("audio overrun samples above threshold: value=%lld threshold=%lld"),
            static_cast<long long>(Result.MetricsSnapshot.AudioOverrunSamples),
            static_cast<long long>(Config.MaxAudioOverrunSamples)
        ));
    }

    if (FailureReasons.Num() == 0)
    {
        Result.Summary = TEXT("PASS");
    }
    else
    {
        Result.Summary = FString::Printf(TEXT("FAIL: %s"), *FString::Join(FailureReasons, TEXT("; ")));
    }

    return Result;
}

void ARetroScreenManager::LogRuntimeQualityGate(const FRetroScreenQualityGateConfig& Config)
{
    UpdateAudioMetrics();
    UpdateWorkerMetrics();
    const FRetroScreenQualityGateResult Result = EvaluateRuntimeQualityGate(Config);
    if (Result.bPassed)
    {
        UE_LOG(
            LogTemp,
            Log,
            TEXT("RetroScreen QualityGate | %s | uploads_ok=%lld uploads_fail=%lld avg=%.3fms peak=%.3fms audio_underrun=%lld audio_overrun=%lld"),
            *Result.Summary,
            static_cast<long long>(Result.MetricsSnapshot.TextureUploadsSucceeded),
            static_cast<long long>(Result.MetricsSnapshot.TextureUploadsFailed),
            Result.MetricsSnapshot.AverageTextureUploadMs,
            Result.MetricsSnapshot.MaxTextureUploadMs,
            static_cast<long long>(Result.MetricsSnapshot.AudioUnderrunSamples),
            static_cast<long long>(Result.MetricsSnapshot.AudioOverrunSamples)
        );
    }
    else
    {
        UE_LOG(
            LogTemp,
            Warning,
            TEXT("RetroScreen QualityGate | %s | uploads_ok=%lld uploads_fail=%lld avg=%.3fms peak=%.3fms audio_underrun=%lld audio_overrun=%lld"),
            *Result.Summary,
            static_cast<long long>(Result.MetricsSnapshot.TextureUploadsSucceeded),
            static_cast<long long>(Result.MetricsSnapshot.TextureUploadsFailed),
            Result.MetricsSnapshot.AverageTextureUploadMs,
            Result.MetricsSnapshot.MaxTextureUploadMs,
            static_cast<long long>(Result.MetricsSnapshot.AudioUnderrunSamples),
            static_cast<long long>(Result.MetricsSnapshot.AudioOverrunSamples)
        );
    }
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
    UpdateWorkerMetrics();
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

    if (InputBridge.IsValid())
    {
        RuntimeMetrics.InputPollCount = static_cast<int64>(InputBridge->GetPollCount());
    }

    RuntimeMetrics.LastMetricsUpdateSeconds = static_cast<float>(FPlatformTime::Seconds());
}

void ARetroScreenManager::LoadRuntimeConfig()
{
    if (RetroScreenConfigPath.TrimStartAndEnd().IsEmpty())
    {
        return;
    }

    FString ResolvedConfigPath = RetroScreenConfigPath;
    if (FPaths::IsRelative(ResolvedConfigPath))
    {
        ResolvedConfigPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), ResolvedConfigPath);
    }

    if (!IFileManager::Get().FileExists(*ResolvedConfigPath))
    {
        return;
    }

    FConfigFile ConfigFile;
    ConfigFile.Read(ResolvedConfigPath);

    static const TCHAR* SectionName = TEXT("RetroScreen");

    ConfigFile.GetBool(SectionName, TEXT("UseUnrealLibretroCore"), bUseUnrealLibretroCore);
    ConfigFile.GetString(SectionName, TEXT("LibretroCorePath"), LibretroCorePath);
    ConfigFile.GetString(SectionName, TEXT("LibretroRomPath"), LibretroRomPath);
    ConfigFile.GetString(SectionName, TEXT("RuntimeRegion"), RuntimeRegion);
    ConfigFile.GetBool(SectionName, TEXT("EnableCrt"), bRuntimeCrtEnabled);
    ConfigFile.GetFloat(SectionName, TEXT("RuntimeAudioVolume"), RuntimeAudioVolume);

    ConfigFile.GetInt(SectionName, TEXT("DefaultJoypadPort"), DefaultJoypadPort);
    ConfigFile.GetInt(SectionName, TEXT("DefaultJoypadDeadzone"), DefaultJoypadDeadzone);
    ConfigFile.GetBool(SectionName, TEXT("DefaultJoypadMapAxesToDpad"), bDefaultJoypadMapAxesToDpad);
    ConfigFile.GetBool(SectionName, TEXT("DefaultJoypadInvertY"), bDefaultJoypadInvertY);

    ConfigFile.GetBool(SectionName, TEXT("LogRuntimeMetrics"), bLogRuntimeMetricsToOutput);
    ConfigFile.GetFloat(SectionName, TEXT("RuntimeMetricsLogIntervalSeconds"), RuntimeMetricsLogIntervalSeconds);

    ConfigFile.GetBool(SectionName, TEXT("ExportRuntimeMetricsCsv"), bExportRuntimeMetricsCsv);
    ConfigFile.GetFloat(SectionName, TEXT("RuntimeMetricsCsvExportIntervalSeconds"), RuntimeMetricsCsvExportIntervalSeconds);
    ConfigFile.GetString(SectionName, TEXT("RuntimeMetricsCsvPath"), RuntimeMetricsCsvPath);

    ConfigFile.GetBool(SectionName, TEXT("LogRuntimeQualityGate"), bLogRuntimeQualityGateToOutput);
    ConfigFile.GetFloat(SectionName, TEXT("RuntimeQualityGateLogIntervalSeconds"), RuntimeQualityGateLogIntervalSeconds);

    ConfigFile.GetInt64(SectionName, TEXT("QualityGateMinTextureUploads"), RuntimeQualityGateConfig.MinTextureUploadsForEvaluation);
    ConfigFile.GetFloat(SectionName, TEXT("QualityGateMaxAverageUploadMs"), RuntimeQualityGateConfig.MaxAverageTextureUploadMs);
    ConfigFile.GetFloat(SectionName, TEXT("QualityGateMaxPeakUploadMs"), RuntimeQualityGateConfig.MaxPeakTextureUploadMs);
    ConfigFile.GetInt64(SectionName, TEXT("QualityGateMaxTextureUploadFailures"), RuntimeQualityGateConfig.MaxTextureUploadFailures);
    ConfigFile.GetInt64(SectionName, TEXT("QualityGateMaxAudioUnderrunSamples"), RuntimeQualityGateConfig.MaxAudioUnderrunSamples);
    ConfigFile.GetInt64(SectionName, TEXT("QualityGateMaxAudioOverrunSamples"), RuntimeQualityGateConfig.MaxAudioOverrunSamples);

    RuntimeMetricsLogIntervalSeconds = FMath::Max(0.1f, RuntimeMetricsLogIntervalSeconds);
    RuntimeMetricsCsvExportIntervalSeconds = FMath::Max(0.1f, RuntimeMetricsCsvExportIntervalSeconds);
    RuntimeQualityGateLogIntervalSeconds = FMath::Max(0.1f, RuntimeQualityGateLogIntervalSeconds);
    RuntimeAudioVolume = FMath::Clamp(RuntimeAudioVolume, 0.0f, 2.0f);
    DefaultJoypadPort = FMath::Max(0, DefaultJoypadPort);
    DefaultJoypadDeadzone = FMath::Clamp(DefaultJoypadDeadzone, 0, static_cast<int32>(MAX_int16));

    RuntimeCoreOptionsAnsi.Reset();
    if (!RuntimeRegion.TrimStartAndEnd().IsEmpty())
    {
        SetRuntimeCoreOption(TEXT("puae_video_standard"), RuntimeRegion);
    }

    SetRuntimeCoreOption(TEXT("puae_gfx_linemode"), bRuntimeCrtEnabled ? TEXT("scanlines") : TEXT("none"));

    static const TCHAR* CoreOptionsSectionName = TEXT("RetroScreenCoreOptions");
    if (const FConfigSection* CoreOptionsSection = ConfigFile.FindSection(CoreOptionsSectionName))
    {
        for (const TPair<FName, FConfigValue>& Pair : *CoreOptionsSection)
        {
            SetRuntimeCoreOption(Pair.Key.ToString(), Pair.Value.GetValue());
        }
    }
}

void ARetroScreenManager::HandleLibretroInputPoll()
{
    if (!InputBridge.IsValid() || !bEmulatorInputEnabled)
    {
        return;
    }

    InputBridge->PollInput();
}

int16 ARetroScreenManager::HandleLibretroInputState(uint32 Port, uint32 Device, uint32 Index, uint32 Id) const
{
    if (!InputBridge.IsValid() || !bEmulatorInputEnabled)
    {
        return 0;
    }

    return InputBridge->GetInputState(Port, Device, Index, Id);
}

bool ARetroScreenManager::HandleLibretroEnvironmentCallback(uint32 Command, void* Data)
{
    switch (Command)
    {
        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
        {
            if (Data == nullptr)
            {
                return false;
            }

            *static_cast<bool*>(Data) = true;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
        {
            if (Data == nullptr)
            {
                return false;
            }

            const retro_pixel_format RequestedFormat = *static_cast<const retro_pixel_format*>(Data);
            const bool bSupported =
                RequestedFormat == RETRO_PIXEL_FORMAT_XRGB8888 ||
                RequestedFormat == RETRO_PIXEL_FORMAT_RGB565;

            if (!bSupported)
            {
                UE_LOG(LogTemp, Warning, TEXT("RetroScreen unsupported libretro pixel format request: %d"), static_cast<int32>(RequestedFormat));
            }

            return bSupported;
        }

        case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
        {
            if (Data == nullptr)
            {
                return false;
            }

            const retro_system_av_info* AvInfo = static_cast<const retro_system_av_info*>(Data);
            if (AvInfo == nullptr)
            {
                return false;
            }

            UE_LOG(
                LogTemp,
                Log,
                TEXT("RetroScreen AV info update | base=%ux%u max=%ux%u fps=%.3f sample_rate=%.3f"),
                AvInfo->geometry.base_width,
                AvInfo->geometry.base_height,
                AvInfo->geometry.max_width,
                AvInfo->geometry.max_height,
                AvInfo->timing.fps,
                AvInfo->timing.sample_rate
            );

            {
                FScopeLock Lock(&MetricsMutex);
                RuntimeMetrics.LastMetricsUpdateSeconds = static_cast<float>(FPlatformTime::Seconds());
            }

            return true;
        }

        case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
        {
            if (Data == nullptr)
            {
                return false;
            }

            retro_log_callback* LogCallback = static_cast<retro_log_callback*>(Data);
            LogCallback->log = RetroScreenCoreLogCallback;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_MESSAGE:
        {
            if (Data == nullptr)
            {
                return false;
            }

            const retro_message* Message = static_cast<const retro_message*>(Data);
            if (Message == nullptr || Message->msg == nullptr)
            {
                return false;
            }

            UE_LOG(LogTemp, Log, TEXT("RetroScreen core message: %s"), UTF8_TO_TCHAR(Message->msg));
            return true;
        }

        case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
        {
            if (Data == nullptr)
            {
                return false;
            }

            *static_cast<const char**>(Data) = RuntimeSystemDirectoryAnsi.c_str();
            return !RuntimeSystemDirectoryAnsi.empty();
        }

        case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
        {
            if (Data == nullptr)
            {
                return false;
            }

            *static_cast<const char**>(Data) = RuntimeSaveDirectoryAnsi.c_str();
            return !RuntimeSaveDirectoryAnsi.empty();
        }

        case RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY:
        {
            if (Data == nullptr)
            {
                return false;
            }

            *static_cast<const char**>(Data) = RuntimeSystemDirectoryAnsi.c_str();
            return !RuntimeSystemDirectoryAnsi.empty();
        }

        case RETRO_ENVIRONMENT_GET_LANGUAGE:
        {
            if (Data == nullptr)
            {
                return false;
            }

            *static_cast<unsigned*>(Data) = RETRO_LANGUAGE_ENGLISH;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
        {
            if (Data == nullptr)
            {
                return false;
            }

            *static_cast<unsigned*>(Data) = 1;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
        {
            if (Data == nullptr)
            {
                return false;
            }

            *static_cast<bool*>(Data) = false;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_TARGET_REFRESH_RATE:
        {
            if (Data == nullptr)
            {
                return false;
            }

            *static_cast<float*>(Data) = 60.0f;
            return true;
        }

        case RETRO_ENVIRONMENT_GET_INPUT_BITMASKS:
        {
            if (Data == nullptr)
            {
                return false;
            }

            *static_cast<bool*>(Data) = true;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_VARIABLES:
        {
            if (Data == nullptr)
            {
                return false;
            }

            const retro_variable* Variable = static_cast<const retro_variable*>(Data);
            while (Variable->key != nullptr)
            {
                const FString Key = UTF8_TO_TCHAR(Variable->key);
                if (!RuntimeCoreOptionsAnsi.Contains(Key))
                {
                    const FString Description = (Variable->value != nullptr) ? UTF8_TO_TCHAR(Variable->value) : FString();
                    const FString DefaultValue = ExtractCoreOptionDefaultValue(Description);
                    SetRuntimeCoreOption(Key, DefaultValue);
                }

                ++Variable;
            }

            return true;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE:
        {
            if (Data == nullptr)
            {
                return false;
            }

            retro_variable* Variable = static_cast<retro_variable*>(Data);
            if (Variable->key == nullptr)
            {
                return false;
            }

            const FString Key = UTF8_TO_TCHAR(Variable->key);
            const std::string* Value = RuntimeCoreOptionsAnsi.Find(Key);
            if (Value == nullptr)
            {
                Variable->value = nullptr;
                return false;
            }

            Variable->value = Value->c_str();
            return true;
        }

        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
        {
            if (Data == nullptr)
            {
                return false;
            }

            *static_cast<bool*>(Data) = false;
            return true;
        }

        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
        {
            if (Data == nullptr)
            {
                return false;
            }

            *static_cast<bool*>(Data) = true;
            return true;
        }

        default:
            return false;
    }
}

void ARetroScreenManager::HandleLibretroVideoRefresh(const uint8* Data, int32 Width, int32 Height, int32 Pitch)
{
    if (!VideoBridge.IsValid() || Data == nullptr || Width <= 0 || Height <= 0 || Pitch <= 0)
    {
        return;
    }

    const uint64 Sequence = VideoBridge->PublishFrame(Data, Width, Height, Pitch);
    if (Sequence > 0)
    {
        FScopeLock Lock(&MetricsMutex);
        RuntimeMetrics.VideoFramesPublished += 1;
        RuntimeMetrics.LastVideoFrameSequence = static_cast<int64>(Sequence);
        RuntimeMetrics.LastMetricsUpdateSeconds = static_cast<float>(FPlatformTime::Seconds());
    }
}

int32 ARetroScreenManager::HandleLibretroAudioSampleBatch(const int16* Samples, int32 Frames)
{
    if (!AudioBridge.IsValid() || Samples == nullptr || Frames <= 0)
    {
        return 0;
    }

    const int32 Channels = 2;
    const int32 NumSamples = Frames * Channels;
    WorkerScratchAudio.SetNumUninitialized(NumSamples);

    for (int32 SampleIndex = 0; SampleIndex < NumSamples; ++SampleIndex)
    {
        WorkerScratchAudio[SampleIndex] =
            (static_cast<float>(Samples[SampleIndex]) / 32768.0f) * RuntimeAudioVolume;
    }

    const int32 SamplesPushed = AudioBridge->PushInterleavedSamples(WorkerScratchAudio.GetData(), NumSamples);
    return SamplesPushed / Channels;
}

void ARetroScreenManager::StepEmulatorFrameOnWorkerThread()
{
    if (LibretroCoreLoader.IsValid() && LibretroCoreLoader->IsLoaded())
    {
        LibretroCoreLoader->RunFrame();
        SyntheticFrameCounter += 1;
        UpdateAudioMetrics();
        return;
    }

    if (!VideoBridge.IsValid() || !AudioBridge.IsValid() || !InputBridge.IsValid())
    {
        return;
    }

    HandleLibretroInputPoll();
    const bool bBoostTone = HandleLibretroInputState(0, 1, 0, 0) != 0;

    const int32 Width = 320;
    const int32 Height = 256;
    const int32 Pitch = Width * 4;

    const int32 PixelCount = Pitch * Height;
    WorkerScratchPixels.SetNumUninitialized(PixelCount);

    const uint8 Scroll = static_cast<uint8>(SyntheticFrameCounter % 255);
    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            const int32 PixelOffset = (Y * Pitch) + (X * 4);
            WorkerScratchPixels[PixelOffset + 0] = static_cast<uint8>((X + Scroll) % 255);         // B
            WorkerScratchPixels[PixelOffset + 1] = static_cast<uint8>(((Y * 2) + Scroll) % 255);   // G
            WorkerScratchPixels[PixelOffset + 2] = static_cast<uint8>((96 + (Scroll / 2)) % 255);  // R
            WorkerScratchPixels[PixelOffset + 3] = 255;                                              // A
        }
    }

    HandleLibretroVideoRefresh(WorkerScratchPixels.GetData(), Width, Height, Pitch);

    const int32 SampleRate = 48000;
    const int32 Channels = 2;
    const int32 FramesPerStep = 960; // 48kHz / 50fps
    const int32 NumSamples = FramesPerStep * Channels;

    WorkerScratchAudioInt16.SetNumUninitialized(NumSamples);

    const double FrequencyHz =
        (bBoostTone ? 330.0 : 220.0) +
        (40.0 * FMath::Sin(static_cast<float>(SyntheticFrameCounter) * 0.05f));
    const double PhaseIncrement = (2.0 * PI * FrequencyHz) / static_cast<double>(SampleRate);

    for (int32 FrameIndex = 0; FrameIndex < FramesPerStep; ++FrameIndex)
    {
        const float SampleValue = 0.15f * FMath::Sin(static_cast<float>(SyntheticAudioPhaseRadians));
        SyntheticAudioPhaseRadians += PhaseIncrement;
        if (SyntheticAudioPhaseRadians > (2.0 * PI))
        {
            SyntheticAudioPhaseRadians -= (2.0 * PI);
        }

        const int16 PcmSample = static_cast<int16>(FMath::Clamp(
            static_cast<int32>(SampleValue * 32767.0f),
            static_cast<int32>(MIN_int16),
            static_cast<int32>(MAX_int16)
        ));

        const int32 SampleOffset = FrameIndex * Channels;
        WorkerScratchAudioInt16[SampleOffset + 0] = PcmSample;
        WorkerScratchAudioInt16[SampleOffset + 1] = PcmSample;
    }

    HandleLibretroAudioSampleBatch(WorkerScratchAudioInt16.GetData(), FramesPerStep);
    SyntheticFrameCounter += 1;

    UpdateAudioMetrics();

    {
        FScopeLock Lock(&MetricsMutex);
        RuntimeMetrics.InputPollCount = static_cast<int64>(InputBridge->GetPollCount());
        RuntimeMetrics.LastMetricsUpdateSeconds = static_cast<float>(FPlatformTime::Seconds());
    }
}

void ARetroScreenManager::UpdateWorkerMetrics()
{
    if (!EmulatorWorker.IsValid())
    {
        return;
    }

    const FRetroScreenEmulatorWorkerStats WorkerStats = EmulatorWorker->GetStats();

    FScopeLock Lock(&MetricsMutex);
    RuntimeMetrics.WorkerFramesExecuted = WorkerStats.TotalFramesExecuted;
    RuntimeMetrics.WorkerLastFrameMs = WorkerStats.LastFrameDurationMs;
    RuntimeMetrics.WorkerAverageFrameMs = WorkerStats.AverageFrameDurationMs;
    RuntimeMetrics.WorkerMaxFrameMs = WorkerStats.MaxFrameDurationMs;
    RuntimeMetrics.LastMetricsUpdateSeconds = static_cast<float>(FPlatformTime::Seconds());
}

void ARetroScreenManager::StartRuntimeMetricsLogging()
{
    if (!bLogRuntimeMetricsToOutput || RuntimeMetricsLogIntervalSeconds <= 0.0f)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    if (World->GetTimerManager().IsTimerActive(RuntimeMetricsLogTimerHandle))
    {
        return;
    }

    World->GetTimerManager().SetTimer(
        RuntimeMetricsLogTimerHandle,
        this,
        &ARetroScreenManager::LogRuntimeMetricsSnapshot,
        RuntimeMetricsLogIntervalSeconds,
        true
    );
}

void ARetroScreenManager::ApplyInteractionInputMode()
{
    bEmulatorInputEnabled = (InteractionInputMode == ERetroScreenInteractionInputMode::Emulator);

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    APlayerController* PlayerController = World->GetFirstPlayerController();
    if (PlayerController == nullptr)
    {
        return;
    }

    if (InteractionInputMode == ERetroScreenInteractionInputMode::Emulator)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(true);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
        PlayerController->SetInputMode(InputMode);
        PlayerController->bShowMouseCursor = false;
        return;
    }

    FInputModeGameAndUI InputMode;
    InputMode.SetHideCursorDuringCapture(false);
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);
    PlayerController->bShowMouseCursor = true;
}

void ARetroScreenManager::SetRuntimeCoreOption(const FString& Key, const FString& Value)
{
    const FString TrimmedKey = Key.TrimStartAndEnd();
    if (TrimmedKey.IsEmpty())
    {
        return;
    }

    const FTCHARToUTF8 ConvertedValue(*Value);
    RuntimeCoreOptionsAnsi.Add(
        TrimmedKey,
        std::string(ConvertedValue.Get(), static_cast<size_t>(ConvertedValue.Length()))
    );
}

void ARetroScreenManager::StopRuntimeMetricsLogging()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(RuntimeMetricsLogTimerHandle);
}

void ARetroScreenManager::StartRuntimeMetricsCsvExport()
{
    if (!bExportRuntimeMetricsCsv || RuntimeMetricsCsvExportIntervalSeconds <= 0.0f || RuntimeMetricsCsvPath.IsEmpty())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    if (World->GetTimerManager().IsTimerActive(RuntimeMetricsCsvTimerHandle))
    {
        return;
    }

    World->GetTimerManager().SetTimer(
        RuntimeMetricsCsvTimerHandle,
        this,
        &ARetroScreenManager::ExportRuntimeMetricsCsvTick,
        RuntimeMetricsCsvExportIntervalSeconds,
        true
    );
}

void ARetroScreenManager::StopRuntimeMetricsCsvExport()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(RuntimeMetricsCsvTimerHandle);
}

void ARetroScreenManager::ExportRuntimeMetricsCsvTick()
{
    if (!bExportRuntimeMetricsCsv || RuntimeMetricsCsvPath.IsEmpty())
    {
        return;
    }

    UpdateAudioMetrics();
    UpdateWorkerMetrics();

    if (!ExportRuntimeMetricsCsv(RuntimeMetricsCsvPath, true))
    {
        UE_LOG(LogTemp, Warning, TEXT("RetroScreen CSV export failed for path: %s"), *RuntimeMetricsCsvPath);
    }
}

void ARetroScreenManager::StartRuntimeQualityGateLogging()
{
    if (!bLogRuntimeQualityGateToOutput || RuntimeQualityGateLogIntervalSeconds <= 0.0f)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    if (World->GetTimerManager().IsTimerActive(RuntimeQualityGateLogTimerHandle))
    {
        return;
    }

    World->GetTimerManager().SetTimer(
        RuntimeQualityGateLogTimerHandle,
        this,
        &ARetroScreenManager::LogRuntimeQualityGateTick,
        RuntimeQualityGateLogIntervalSeconds,
        true
    );
}

void ARetroScreenManager::StopRuntimeQualityGateLogging()
{
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(RuntimeQualityGateLogTimerHandle);
}

void ARetroScreenManager::LogRuntimeQualityGateTick()
{
    if (!bLogRuntimeQualityGateToOutput)
    {
        return;
    }

    LogRuntimeQualityGate(RuntimeQualityGateConfig);
}
