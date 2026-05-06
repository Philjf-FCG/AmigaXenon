#pragma once

#include "CoreMinimal.h"

class RETROSCREEN_API FRetroScreenAudioBridge
{
public:
    FRetroScreenAudioBridge();

    void Initialize(int32 InCapacitySamples);
    void Reset();

    int32 PushInterleavedSamples(const float* InSamples, int32 NumSamples);
    int32 PopInterleavedSamples(float* OutSamples, int32 NumSamples);

    int32 GetAvailableSamples() const;
    uint64 GetUnderrunCount() const { return UnderrunCount; }
    uint64 GetOverrunCount() const { return OverrunCount; }
    uint64 GetTotalSamplesPushed() const { return TotalSamplesPushed; }
    uint64 GetTotalSamplesPopped() const { return TotalSamplesPopped; }

private:
    mutable FCriticalSection BufferMutex;

    TArray<float> Buffer;
    int32 CapacitySamples;
    int32 ReadCursor;
    int32 WriteCursor;
    int32 AvailableSamples;

    uint64 UnderrunCount;
    uint64 OverrunCount;
    uint64 TotalSamplesPushed;
    uint64 TotalSamplesPopped;
};
