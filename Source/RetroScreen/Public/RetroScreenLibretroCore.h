#pragma once

#include "CoreMinimal.h"
#include "libretro/libretro.h"

struct FRetroScreenLibretroCallbacks
{
    TFunction<bool(uint32, void*)> EnvironmentCallback;
    TFunction<void(const uint8*, int32, int32, int32)> VideoRefreshCallback;
    TFunction<int32(const int16*, int32)> AudioSampleBatchCallback;
    TFunction<void()> InputPollCallback;
    TFunction<int16(uint32, uint32, uint32, uint32)> InputStateCallback;
};

class RETROSCREEN_API FRetroScreenLibretroCore
{
public:
    FRetroScreenLibretroCore();
    ~FRetroScreenLibretroCore();

    bool LoadCore(const FString& CorePath, const FRetroScreenLibretroCallbacks& InCallbacks, FString& OutError);
    bool LoadGame(const FString& RomPath, FString& OutError);
    void RunFrame();
    void Shutdown();

    bool IsLoaded() const { return bCoreLoaded; }

private:
    bool LoadRequiredSymbols(FString& OutError);
    void ResetFunctionPointers();

    static bool RETRO_CALLCONV StaticEnvironmentCallback(unsigned Command, void* Data);
    static void RETRO_CALLCONV StaticVideoRefreshCallback(const void* Data, unsigned Width, unsigned Height, size_t Pitch);
    static size_t RETRO_CALLCONV StaticAudioSampleBatchCallback(const int16_t* Data, size_t Frames);
    static void RETRO_CALLCONV StaticInputPollCallback();
    static int16_t RETRO_CALLCONV StaticInputStateCallback(unsigned Port, unsigned Device, unsigned Index, unsigned Id);

    static FRetroScreenLibretroCore* ActiveInstance;

    FRetroScreenLibretroCallbacks Callbacks;
    void* CoreHandle;
    bool bCoreLoaded;
    bool bGameLoaded;

    void (RETRO_CALLCONV* RetroSetEnvironment)(retro_environment_t);
    void (RETRO_CALLCONV* RetroSetVideoRefresh)(retro_video_refresh_t);
    void (RETRO_CALLCONV* RetroSetAudioSampleBatch)(retro_audio_sample_batch_t);
    void (RETRO_CALLCONV* RetroSetInputPoll)(retro_input_poll_t);
    void (RETRO_CALLCONV* RetroSetInputState)(retro_input_state_t);

    void (RETRO_CALLCONV* RetroInit)();
    void (RETRO_CALLCONV* RetroDeinit)();
    bool (RETRO_CALLCONV* RetroLoadGame)(const retro_game_info*);
    void (RETRO_CALLCONV* RetroUnloadGame)();
    void (RETRO_CALLCONV* RetroRun)();
};
