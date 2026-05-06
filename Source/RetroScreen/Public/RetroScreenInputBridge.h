#pragma once

#include "CoreMinimal.h"

class RETROSCREEN_API FRetroScreenInputBridge
{
public:
    void SetDigitalState(uint32 Port, uint32 Device, uint32 Index, uint32 Id, bool bPressed);
    void SetAnalogState(uint32 Port, uint32 Device, uint32 Index, uint32 Id, int16 Value);
    void SetJoypadButton(uint32 Port, uint32 ButtonId, bool bPressed);
    void SetJoypadAxes(
        uint32 Port,
        int16 LeftX,
        int16 LeftY,
        int16 Deadzone = 8192,
        bool bMapAxesToDpad = true
    );

    int16 GetInputState(uint32 Port, uint32 Device, uint32 Index, uint32 Id) const;
    void PollInput();

    uint64 GetPollCount() const;
    void ClearStates();
    void Reset();

private:
    static uint32 MakeInputKey(uint32 Port, uint32 Device, uint32 Index, uint32 Id);

    mutable FRWLock StateLock;
    TMap<uint32, int16> InputStates;
    TAtomic<uint64> PollCount{0};
};
