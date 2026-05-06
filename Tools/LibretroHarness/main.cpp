#include <windows.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include "libretro/libretro.h"

namespace
{
struct RetroApi
{
    void (*set_environment)(retro_environment_t) = nullptr;
    void (*set_video_refresh)(retro_video_refresh_t) = nullptr;
    void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = nullptr;
    void (*set_input_poll)(retro_input_poll_t) = nullptr;
    void (*set_input_state)(retro_input_state_t) = nullptr;

    void (*init)() = nullptr;
    void (*deinit)() = nullptr;
    bool (*load_game)(const retro_game_info*) = nullptr;
    void (*unload_game)() = nullptr;
    void (*run)() = nullptr;
};

struct CallbackState
{
    uint64_t VideoFrameCount = 0;
    uint64_t AudioFrameCount = 0;
    uint64_t InputPollCount = 0;
    unsigned LastWidth = 0;
    unsigned LastHeight = 0;
    size_t LastPitch = 0;
    retro_pixel_format PixelFormat = RETRO_PIXEL_FORMAT_UNKNOWN;
};

CallbackState GState;
std::map<std::string, std::string> GCoreOptions;

std::string WStringToUtf8(const std::wstring& Text)
{
    if (Text.empty())
    {
        return std::string();
    }

    const int RequiredSize = WideCharToMultiByte(
        CP_UTF8,
        0,
        Text.c_str(),
        static_cast<int>(Text.size()),
        nullptr,
        0,
        nullptr,
        nullptr
    );

    if (RequiredSize <= 0)
    {
        return std::string();
    }

    std::string Utf8;
    Utf8.resize(static_cast<size_t>(RequiredSize));

    const int ConvertedSize = WideCharToMultiByte(
        CP_UTF8,
        0,
        Text.c_str(),
        static_cast<int>(Text.size()),
        Utf8.data(),
        RequiredSize,
        nullptr,
        nullptr
    );

    if (ConvertedSize <= 0)
    {
        return std::string();
    }

    return Utf8;
}

std::string ExtractDefaultVariableValue(const char* Description)
{
    if (Description == nullptr)
    {
        return std::string();
    }

    std::string Raw(Description);
    const size_t SemicolonPos = Raw.find(';');
    if (SemicolonPos == std::string::npos)
    {
        return std::string();
    }

    std::string Choices = Raw.substr(SemicolonPos + 1);
    while (!Choices.empty() && (Choices.front() == ' ' || Choices.front() == '\t'))
    {
        Choices.erase(Choices.begin());
    }

    const size_t PipePos = Choices.find('|');
    if (PipePos != std::string::npos)
    {
        Choices = Choices.substr(0, PipePos);
    }

    while (!Choices.empty() && (Choices.back() == ' ' || Choices.back() == '\t'))
    {
        Choices.pop_back();
    }

    return Choices;
}

const char* PixelFormatToString(retro_pixel_format Format)
{
    switch (Format)
    {
        case RETRO_PIXEL_FORMAT_0RGB1555:
            return "0RGB1555";
        case RETRO_PIXEL_FORMAT_XRGB8888:
            return "XRGB8888";
        case RETRO_PIXEL_FORMAT_RGB565:
            return "RGB565";
        default:
            return "UNKNOWN";
    }
}

std::string EscapeJsonString(const std::string& Value)
{
    std::string Escaped;
    Escaped.reserve(Value.size() + 16);

    for (char Ch : Value)
    {
        switch (Ch)
        {
            case '\\': Escaped += "\\\\"; break;
            case '"': Escaped += "\\\""; break;
            case '\n': Escaped += "\\n"; break;
            case '\r': Escaped += "\\r"; break;
            case '\t': Escaped += "\\t"; break;
            default: Escaped += Ch; break;
        }
    }

    return Escaped;
}

void WriteJsonSummary(
    const std::filesystem::path& OutputPath,
    const std::filesystem::path& CorePath,
    const std::filesystem::path& RomPath,
    int FrameCount,
    double DurationMs
)
{
    std::ofstream OutFile(OutputPath, std::ios::out | std::ios::trunc);
    if (!OutFile.is_open())
    {
        std::wcerr << L"Failed to open output file: " << OutputPath << L"\n";
        return;
    }

    const double EffectiveFps = DurationMs > 0.0 ? (1000.0 * static_cast<double>(FrameCount) / DurationMs) : 0.0;

    OutFile << "{\n";
    OutFile << "  \"core_path\": \"" << EscapeJsonString(CorePath.string()) << "\",\n";
    OutFile << "  \"rom_path\": \"" << EscapeJsonString(RomPath.string()) << "\",\n";
    OutFile << "  \"frames_requested\": " << FrameCount << ",\n";
    OutFile << "  \"duration_ms\": " << DurationMs << ",\n";
    OutFile << "  \"effective_fps\": " << EffectiveFps << ",\n";
    OutFile << "  \"video_callback_frames\": " << GState.VideoFrameCount << ",\n";
    OutFile << "  \"audio_callback_frames\": " << GState.AudioFrameCount << ",\n";
    OutFile << "  \"input_poll_count\": " << GState.InputPollCount << ",\n";
    OutFile << "  \"last_width\": " << GState.LastWidth << ",\n";
    OutFile << "  \"last_height\": " << GState.LastHeight << ",\n";
    OutFile << "  \"last_pitch\": " << GState.LastPitch << ",\n";
    OutFile << "  \"pixel_format\": \"" << PixelFormatToString(GState.PixelFormat) << "\",\n";
    OutFile << "  \"core_options_count\": " << GCoreOptions.size() << "\n";
    OutFile << "}\n";
}

bool RETRO_CALLCONV EnvironmentCallback(unsigned Command, void* Data)
{
    switch (Command)
    {
        case RETRO_ENVIRONMENT_GET_CAN_DUPE:
            if (Data != nullptr)
            {
                *static_cast<bool*>(Data) = true;
                return true;
            }
            return false;

        case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
            if (Data != nullptr)
            {
                GState.PixelFormat = *static_cast<const retro_pixel_format*>(Data);
                return GState.PixelFormat == RETRO_PIXEL_FORMAT_XRGB8888 ||
                       GState.PixelFormat == RETRO_PIXEL_FORMAT_RGB565;
            }
            return false;

        case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
            return true;

        case RETRO_ENVIRONMENT_SET_VARIABLES:
            if (Data == nullptr)
            {
                return false;
            }

            for (const retro_variable* Variable = static_cast<const retro_variable*>(Data); Variable->key != nullptr; ++Variable)
            {
                if (GCoreOptions.find(Variable->key) == GCoreOptions.end())
                {
                    GCoreOptions[Variable->key] = ExtractDefaultVariableValue(Variable->value);
                }
            }

            return true;

        case RETRO_ENVIRONMENT_GET_VARIABLE:
            if (Data == nullptr)
            {
                return false;
            }
            else
            {
                retro_variable* Variable = static_cast<retro_variable*>(Data);
                if (Variable->key == nullptr)
                {
                    return false;
                }

                const auto It = GCoreOptions.find(Variable->key);
                if (It == GCoreOptions.end())
                {
                    Variable->value = nullptr;
                    return false;
                }

                Variable->value = It->second.c_str();
                return true;
            }

        case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
            if (Data != nullptr)
            {
                *static_cast<bool*>(Data) = false;
                return true;
            }
            return false;

        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
            if (Data != nullptr)
            {
                *static_cast<bool*>(Data) = true;
                return true;
            }
            return false;

        default:
            return false;
    }
}

void RETRO_CALLCONV VideoRefreshCallback(const void* Data, unsigned Width, unsigned Height, size_t Pitch)
{
    if (Data == nullptr || Data == RETRO_HW_FRAME_BUFFER_VALID)
    {
        return;
    }

    GState.VideoFrameCount += 1;
    GState.LastWidth = Width;
    GState.LastHeight = Height;
    GState.LastPitch = Pitch;
}

size_t RETRO_CALLCONV AudioSampleBatchCallback(const int16_t* Data, size_t Frames)
{
    if (Data == nullptr)
    {
        return 0;
    }

    GState.AudioFrameCount += Frames;
    return Frames;
}

void RETRO_CALLCONV InputPollCallback()
{
    GState.InputPollCount += 1;
}

int16_t RETRO_CALLCONV InputStateCallback(unsigned, unsigned, unsigned, unsigned)
{
    return 0;
}

template <typename T>
T LoadSymbol(HMODULE ModuleHandle, const char* Name)
{
    FARPROC Symbol = GetProcAddress(ModuleHandle, Name);
    return reinterpret_cast<T>(Symbol);
}

bool ResolveApi(HMODULE ModuleHandle, RetroApi& Api)
{
    Api.set_environment = LoadSymbol<decltype(Api.set_environment)>(ModuleHandle, "retro_set_environment");
    Api.set_video_refresh = LoadSymbol<decltype(Api.set_video_refresh)>(ModuleHandle, "retro_set_video_refresh");
    Api.set_audio_sample_batch = LoadSymbol<decltype(Api.set_audio_sample_batch)>(ModuleHandle, "retro_set_audio_sample_batch");
    Api.set_input_poll = LoadSymbol<decltype(Api.set_input_poll)>(ModuleHandle, "retro_set_input_poll");
    Api.set_input_state = LoadSymbol<decltype(Api.set_input_state)>(ModuleHandle, "retro_set_input_state");

    Api.init = LoadSymbol<decltype(Api.init)>(ModuleHandle, "retro_init");
    Api.deinit = LoadSymbol<decltype(Api.deinit)>(ModuleHandle, "retro_deinit");
    Api.load_game = LoadSymbol<decltype(Api.load_game)>(ModuleHandle, "retro_load_game");
    Api.unload_game = LoadSymbol<decltype(Api.unload_game)>(ModuleHandle, "retro_unload_game");
    Api.run = LoadSymbol<decltype(Api.run)>(ModuleHandle, "retro_run");

    return Api.set_environment != nullptr &&
           Api.set_video_refresh != nullptr &&
           Api.set_audio_sample_batch != nullptr &&
           Api.set_input_poll != nullptr &&
           Api.set_input_state != nullptr &&
           Api.init != nullptr &&
           Api.deinit != nullptr &&
           Api.load_game != nullptr &&
           Api.unload_game != nullptr &&
           Api.run != nullptr;
}

int ParseFrameCount(const std::wstring& Text)
{
    try
    {
        const int Value = std::stoi(Text);
        return Value > 0 ? Value : 300;
    }
    catch (...)
    {
        return 300;
    }
}
} // namespace

int wmain(int Argc, wchar_t** Argv)
{
    if (Argc < 2)
    {
        std::wcerr << L"Usage: libretro_harness <core_path> [rom_path] [frame_count] [--option key=value] [--kickstart path] [--rom path] [--frames N]\n";
        return 1;
    }

    const std::filesystem::path CorePath = std::filesystem::path(Argv[1]);
    std::filesystem::path RomPath = Argc >= 3 && std::wstring(Argv[2]).rfind(L"--", 0) != 0 ? std::filesystem::path(Argv[2]) : std::filesystem::path();
    int FrameCount = Argc >= 4 && std::wstring(Argv[3]).rfind(L"--", 0) != 0 ? ParseFrameCount(Argv[3]) : 300;
    std::filesystem::path OutputPath;

    int OptionArgStart = 2;
    if (!RomPath.empty())
    {
        OptionArgStart = 3;
    }
    if (OptionArgStart < Argc && std::wstring(Argv[OptionArgStart]).rfind(L"--", 0) != 0)
    {
        OptionArgStart += 1;
    }

    for (int ArgIndex = OptionArgStart; ArgIndex < Argc; ++ArgIndex)
    {
        const std::wstring Arg = Argv[ArgIndex];

        if (Arg == L"--option" && (ArgIndex + 1) < Argc)
        {
            const std::wstring Pair = Argv[++ArgIndex];
            const size_t EqualPos = Pair.find(L'=');
            if (EqualPos != std::wstring::npos)
            {
                const std::wstring Key = Pair.substr(0, EqualPos);
                const std::wstring Value = Pair.substr(EqualPos + 1);
                GCoreOptions[WStringToUtf8(Key)] = WStringToUtf8(Value);
            }
            continue;
        }

        if (Arg == L"--kickstart" && (ArgIndex + 1) < Argc)
        {
            const std::wstring KickstartPath = Argv[++ArgIndex];
            const std::string KickstartUtf8 = WStringToUtf8(KickstartPath);
            GCoreOptions["puae_rom"] = KickstartUtf8;
            GCoreOptions["kickstart_path"] = KickstartUtf8;
            continue;
        }

        if (Arg == L"--rom" && (ArgIndex + 1) < Argc)
        {
            RomPath = std::filesystem::path(Argv[++ArgIndex]);
            continue;
        }

        if (Arg == L"--frames" && (ArgIndex + 1) < Argc)
        {
            FrameCount = ParseFrameCount(Argv[++ArgIndex]);
            continue;
        }

        if (Arg == L"--output" && (ArgIndex + 1) < Argc)
        {
            OutputPath = std::filesystem::path(Argv[++ArgIndex]);
            continue;
        }
    }

    if (!std::filesystem::exists(CorePath))
    {
        std::wcerr << L"Core path does not exist: " << CorePath << L"\n";
        return 2;
    }

    HMODULE CoreModule = LoadLibraryW(CorePath.c_str());
    if (CoreModule == nullptr)
    {
        std::wcerr << L"Failed to load core DLL: " << CorePath << L"\n";
        return 3;
    }

    RetroApi Api;
    if (!ResolveApi(CoreModule, Api))
    {
        std::wcerr << L"Failed to resolve one or more required retro_* symbols.\n";
        FreeLibrary(CoreModule);
        return 4;
    }

    Api.set_environment(EnvironmentCallback);
    Api.set_video_refresh(VideoRefreshCallback);
    Api.set_audio_sample_batch(AudioSampleBatchCallback);
    Api.set_input_poll(InputPollCallback);
    Api.set_input_state(InputStateCallback);

    Api.init();

    std::string RomUtf8;
    retro_game_info GameInfo{};
    retro_game_info* GameInfoPtr = nullptr;

    if (!RomPath.empty())
    {
        if (!std::filesystem::exists(RomPath))
        {
            std::wcerr << L"ROM path does not exist: " << RomPath << L"\n";
            Api.deinit();
            FreeLibrary(CoreModule);
            return 5;
        }

        RomUtf8 = RomPath.string();
        GameInfo.path = RomUtf8.c_str();
        GameInfo.data = nullptr;
        GameInfo.size = 0;
        GameInfo.meta = nullptr;
        GameInfoPtr = &GameInfo;
    }

    if (!Api.load_game(GameInfoPtr))
    {
        std::wcerr << L"retro_load_game failed.\n";
        Api.deinit();
        FreeLibrary(CoreModule);
        return 6;
    }

    const auto StartTime = std::chrono::steady_clock::now();
    for (int FrameIndex = 0; FrameIndex < FrameCount; ++FrameIndex)
    {
        Api.run();
    }
    const auto EndTime = std::chrono::steady_clock::now();

    const double DurationMs =
        std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(EndTime - StartTime).count();

    std::wcout << L"Harness complete\n";
    std::wcout << L"Frames requested: " << FrameCount << L"\n";
    std::wcout << L"Video callback frames: " << GState.VideoFrameCount << L"\n";
    std::wcout << L"Audio callback frames: " << GState.AudioFrameCount << L"\n";
    std::wcout << L"Input poll count: " << GState.InputPollCount << L"\n";
    std::wcout << L"Last frame geometry: " << GState.LastWidth << L"x" << GState.LastHeight << L" pitch=" << GState.LastPitch << L"\n";
    std::wcout << L"Duration (ms): " << DurationMs << L"\n";
    std::wcout << L"Core options configured: " << GCoreOptions.size() << L"\n";

    if (!OutputPath.empty())
    {
        WriteJsonSummary(OutputPath, CorePath, RomPath, FrameCount, DurationMs);
    }

    Api.unload_game();
    Api.deinit();
    FreeLibrary(CoreModule);
    return 0;
}
