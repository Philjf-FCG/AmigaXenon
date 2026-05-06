#include "RetroScreenInputBridge.h"

#include "libretro/libretro.h"

uint32 FRetroScreenInputBridge::MakeInputKey(uint32 Port, uint32 Device, uint32 Index, uint32 Id)
{
    const uint32 PackedPort = (Port & 0xFFu) << 24;
    const uint32 PackedDevice = (Device & 0xFFu) << 16;
    const uint32 PackedIndex = (Index & 0xFFu) << 8;
    const uint32 PackedId = (Id & 0xFFu);
    return PackedPort | PackedDevice | PackedIndex | PackedId;
}

void FRetroScreenInputBridge::SetDigitalState(uint32 Port, uint32 Device, uint32 Index, uint32 Id, bool bPressed)
{
    SetAnalogState(Port, Device, Index, Id, bPressed ? 1 : 0);
}

void FRetroScreenInputBridge::SetAnalogState(uint32 Port, uint32 Device, uint32 Index, uint32 Id, int16 Value)
{
    const uint32 Key = MakeInputKey(Port, Device, Index, Id);

    FWriteScopeLock Lock(StateLock);
    InputStates.FindOrAdd(Key) = Value;
}

void FRetroScreenInputBridge::SetJoypadButton(uint32 Port, uint32 ButtonId, bool bPressed)
{
    SetDigitalState(Port, RETRO_DEVICE_JOYPAD, 0, ButtonId, bPressed);
}

void FRetroScreenInputBridge::SetJoypadAxes(uint32 Port, int16 LeftX, int16 LeftY, int16 Deadzone, bool bMapAxesToDpad)
{
    const int16 ClampedDeadzone = FMath::Clamp<int16>(Deadzone, 0, MAX_int16);

    // Left analog stick exposed via RETRO_DEVICE_ANALOG on index 0.
    SetAnalogState(Port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, LeftX);
    SetAnalogState(Port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, LeftY);

    if (!bMapAxesToDpad)
    {
        return;
    }

    // Optional analog-to-digital conversion for cores that only query joypad dpad buttons.
    SetJoypadButton(Port, RETRO_DEVICE_ID_JOYPAD_LEFT, LeftX < -ClampedDeadzone);
    SetJoypadButton(Port, RETRO_DEVICE_ID_JOYPAD_RIGHT, LeftX > ClampedDeadzone);
    SetJoypadButton(Port, RETRO_DEVICE_ID_JOYPAD_UP, LeftY < -ClampedDeadzone);
    SetJoypadButton(Port, RETRO_DEVICE_ID_JOYPAD_DOWN, LeftY > ClampedDeadzone);
}

int16 FRetroScreenInputBridge::GetInputState(uint32 Port, uint32 Device, uint32 Index, uint32 Id) const
{
    const uint32 Key = MakeInputKey(Port, Device, Index, Id);

    FReadScopeLock Lock(StateLock);
    const int16* ValuePtr = InputStates.Find(Key);
    return (ValuePtr != nullptr) ? *ValuePtr : 0;
}

void FRetroScreenInputBridge::PollInput()
{
    ++PollCount;
}

uint64 FRetroScreenInputBridge::GetPollCount() const
{
    return PollCount.Load(EMemoryOrder::Relaxed);
}

void FRetroScreenInputBridge::ClearStates()
{
    FWriteScopeLock Lock(StateLock);
    InputStates.Reset();
}

void FRetroScreenInputBridge::Reset()
{
    ClearStates();

    PollCount.Store(0, EMemoryOrder::Relaxed);
}
