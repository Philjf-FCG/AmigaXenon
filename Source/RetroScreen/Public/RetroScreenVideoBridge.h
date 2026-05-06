#pragma once

#include "CoreMinimal.h"

struct FRetroScreenVideoFrame
{
    int32 Width = 0;
    int32 Height = 0;
    int32 Pitch = 0;
    uint64 Sequence = 0;
    TArray<uint8> Pixels;

    bool IsValid() const
    {
        return Width > 0 && Height > 0 && Pitch > 0 && Pixels.Num() > 0;
    }
};

class RETROSCREEN_API FRetroScreenVideoBridge
{
public:
    FRetroScreenVideoBridge();

    uint64 PublishFrame(const uint8* InPixels, int32 InWidth, int32 InHeight, int32 InPitch);
    bool ConsumeLatestFrame(FRetroScreenVideoFrame& OutFrame);
    void Reset();

private:
    FCriticalSection BufferMutex;
    FRetroScreenVideoFrame Buffers[2];
    int32 LatestBufferIndex;
    bool bHasFrame;
    uint64 NextSequence;
};
