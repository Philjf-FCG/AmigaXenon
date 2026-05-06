#include "RetroScreenLibretroCore.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"

FRetroScreenLibretroCore* FRetroScreenLibretroCore::ActiveInstance = nullptr;

FRetroScreenLibretroCore::FRetroScreenLibretroCore()
    : CoreHandle(nullptr)
    , bCoreLoaded(false)
    , bGameLoaded(false)
{
    ResetFunctionPointers();
}

FRetroScreenLibretroCore::~FRetroScreenLibretroCore()
{
    Shutdown();
}

bool FRetroScreenLibretroCore::LoadCore(const FString& CorePath, const FRetroScreenLibretroCallbacks& InCallbacks, FString& OutError)
{
    Shutdown();

    if (CorePath.TrimStartAndEnd().IsEmpty())
    {
        OutError = TEXT("Core path is empty.");
        return false;
    }

    FString ResolvedCorePath = CorePath;
    if (FPaths::IsRelative(ResolvedCorePath))
    {
        ResolvedCorePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), ResolvedCorePath);
    }

    if (!IFileManager::Get().FileExists(*ResolvedCorePath))
    {
        OutError = FString::Printf(TEXT("Core file does not exist: %s"), *ResolvedCorePath);
        return false;
    }

    CoreHandle = FPlatformProcess::GetDllHandle(*ResolvedCorePath);
    if (CoreHandle == nullptr)
    {
        OutError = FString::Printf(TEXT("Failed to load core library: %s"), *ResolvedCorePath);
        return false;
    }

    if (!LoadRequiredSymbols(OutError))
    {
        Shutdown();
        return false;
    }

    Callbacks = InCallbacks;
    ActiveInstance = this;

    RetroSetEnvironment(StaticEnvironmentCallback);
    RetroSetVideoRefresh(StaticVideoRefreshCallback);
    RetroSetAudioSampleBatch(StaticAudioSampleBatchCallback);
    RetroSetInputPoll(StaticInputPollCallback);
    RetroSetInputState(StaticInputStateCallback);

    RetroInit();
    bCoreLoaded = true;
    bGameLoaded = false;

    return true;
}

bool FRetroScreenLibretroCore::LoadGame(const FString& RomPath, FString& OutError)
{
    if (!bCoreLoaded || RetroLoadGame == nullptr)
    {
        OutError = TEXT("Core is not loaded.");
        return false;
    }

    if (bGameLoaded && RetroUnloadGame != nullptr)
    {
        RetroUnloadGame();
        bGameLoaded = false;
    }

    if (RomPath.TrimStartAndEnd().IsEmpty())
    {
        const bool bLoadedNoGame = RetroLoadGame(nullptr);
        if (!bLoadedNoGame)
        {
            OutError = TEXT("retro_load_game(nullptr) failed.");
        }

        bGameLoaded = bLoadedNoGame;
        return bLoadedNoGame;
    }

    FString ResolvedRomPath = RomPath;
    if (FPaths::IsRelative(ResolvedRomPath))
    {
        ResolvedRomPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), ResolvedRomPath);
    }

    if (!IFileManager::Get().FileExists(*ResolvedRomPath) && !IFileManager::Get().DirectoryExists(*ResolvedRomPath))
    {
        OutError = FString::Printf(TEXT("ROM path does not exist: %s"), *ResolvedRomPath);
        return false;
    }

    FTCHARToUTF8 RomPathUtf8(*ResolvedRomPath);

    retro_game_info GameInfo{};
    GameInfo.path = RomPathUtf8.Get();
    GameInfo.data = nullptr;
    GameInfo.size = 0;
    GameInfo.meta = nullptr;

    const bool bLoaded = RetroLoadGame(&GameInfo);
    if (!bLoaded)
    {
        OutError = FString::Printf(TEXT("retro_load_game failed for path: %s"), *ResolvedRomPath);
        return false;
    }

    bGameLoaded = true;
    return true;
}

void FRetroScreenLibretroCore::RunFrame()
{
    if (bCoreLoaded && RetroRun != nullptr)
    {
        RetroRun();
    }
}

void FRetroScreenLibretroCore::Shutdown()
{
    if (ActiveInstance == this)
    {
        ActiveInstance = nullptr;
    }

    if (bCoreLoaded)
    {
        if (bGameLoaded && RetroUnloadGame != nullptr)
        {
            RetroUnloadGame();
        }

        if (RetroDeinit != nullptr)
        {
            RetroDeinit();
        }
    }

    bCoreLoaded = false;
    bGameLoaded = false;

    if (CoreHandle != nullptr)
    {
        FPlatformProcess::FreeDllHandle(CoreHandle);
        CoreHandle = nullptr;
    }

    ResetFunctionPointers();
    Callbacks = FRetroScreenLibretroCallbacks();
}

bool FRetroScreenLibretroCore::LoadRequiredSymbols(FString& OutError)
{
    auto LoadSymbol = [this, &OutError](const TCHAR* Name) -> void*
    {
        void* Symbol = FPlatformProcess::GetDllExport(CoreHandle, Name);
        if (Symbol == nullptr)
        {
            OutError = FString::Printf(TEXT("Missing required symbol: %s"), Name);
        }

        return Symbol;
    };

    RetroSetEnvironment = reinterpret_cast<decltype(RetroSetEnvironment)>(LoadSymbol(TEXT("retro_set_environment")));
    if (RetroSetEnvironment == nullptr) return false;

    RetroSetVideoRefresh = reinterpret_cast<decltype(RetroSetVideoRefresh)>(LoadSymbol(TEXT("retro_set_video_refresh")));
    if (RetroSetVideoRefresh == nullptr) return false;

    RetroSetAudioSampleBatch = reinterpret_cast<decltype(RetroSetAudioSampleBatch)>(LoadSymbol(TEXT("retro_set_audio_sample_batch")));
    if (RetroSetAudioSampleBatch == nullptr) return false;

    RetroSetInputPoll = reinterpret_cast<decltype(RetroSetInputPoll)>(LoadSymbol(TEXT("retro_set_input_poll")));
    if (RetroSetInputPoll == nullptr) return false;

    RetroSetInputState = reinterpret_cast<decltype(RetroSetInputState)>(LoadSymbol(TEXT("retro_set_input_state")));
    if (RetroSetInputState == nullptr) return false;

    RetroInit = reinterpret_cast<decltype(RetroInit)>(LoadSymbol(TEXT("retro_init")));
    if (RetroInit == nullptr) return false;

    RetroDeinit = reinterpret_cast<decltype(RetroDeinit)>(LoadSymbol(TEXT("retro_deinit")));
    if (RetroDeinit == nullptr) return false;

    RetroLoadGame = reinterpret_cast<decltype(RetroLoadGame)>(LoadSymbol(TEXT("retro_load_game")));
    if (RetroLoadGame == nullptr) return false;

    RetroUnloadGame = reinterpret_cast<decltype(RetroUnloadGame)>(LoadSymbol(TEXT("retro_unload_game")));
    if (RetroUnloadGame == nullptr) return false;

    RetroRun = reinterpret_cast<decltype(RetroRun)>(LoadSymbol(TEXT("retro_run")));
    if (RetroRun == nullptr) return false;

    return true;
}

void FRetroScreenLibretroCore::ResetFunctionPointers()
{
    RetroSetEnvironment = nullptr;
    RetroSetVideoRefresh = nullptr;
    RetroSetAudioSampleBatch = nullptr;
    RetroSetInputPoll = nullptr;
    RetroSetInputState = nullptr;
    RetroInit = nullptr;
    RetroDeinit = nullptr;
    RetroLoadGame = nullptr;
    RetroUnloadGame = nullptr;
    RetroRun = nullptr;
}

bool RETRO_CALLCONV FRetroScreenLibretroCore::StaticEnvironmentCallback(unsigned Command, void* Data)
{
    if (ActiveInstance == nullptr || !ActiveInstance->Callbacks.EnvironmentCallback)
    {
        return false;
    }

    return ActiveInstance->Callbacks.EnvironmentCallback(static_cast<uint32>(Command), Data);
}

void RETRO_CALLCONV FRetroScreenLibretroCore::StaticVideoRefreshCallback(const void* Data, unsigned Width, unsigned Height, size_t Pitch)
{
    if (ActiveInstance == nullptr || !ActiveInstance->Callbacks.VideoRefreshCallback)
    {
        return;
    }

    if (Data == nullptr || Data == RETRO_HW_FRAME_BUFFER_VALID)
    {
        return;
    }

    ActiveInstance->Callbacks.VideoRefreshCallback(
        static_cast<const uint8*>(Data),
        static_cast<int32>(Width),
        static_cast<int32>(Height),
        static_cast<int32>(Pitch)
    );
}

size_t RETRO_CALLCONV FRetroScreenLibretroCore::StaticAudioSampleBatchCallback(const int16_t* Data, size_t Frames)
{
    if (ActiveInstance == nullptr || !ActiveInstance->Callbacks.AudioSampleBatchCallback || Data == nullptr)
    {
        return 0;
    }

    const int32 RequestedFrames = static_cast<int32>(FMath::Min<size_t>(Frames, static_cast<size_t>(MAX_int32)));
    const int32 ConsumedFrames = ActiveInstance->Callbacks.AudioSampleBatchCallback(Data, RequestedFrames);
    return static_cast<size_t>(FMath::Max(0, ConsumedFrames));
}

void RETRO_CALLCONV FRetroScreenLibretroCore::StaticInputPollCallback()
{
    if (ActiveInstance == nullptr || !ActiveInstance->Callbacks.InputPollCallback)
    {
        return;
    }

    ActiveInstance->Callbacks.InputPollCallback();
}

int16_t RETRO_CALLCONV FRetroScreenLibretroCore::StaticInputStateCallback(unsigned Port, unsigned Device, unsigned Index, unsigned Id)
{
    if (ActiveInstance == nullptr || !ActiveInstance->Callbacks.InputStateCallback)
    {
        return 0;
    }

    return ActiveInstance->Callbacks.InputStateCallback(
        static_cast<uint32>(Port),
        static_cast<uint32>(Device),
        static_cast<uint32>(Index),
        static_cast<uint32>(Id)
    );
}
