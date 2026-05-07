#include "RetroScreenAudioBridge.h"

FRetroScreenAudioBridge::FRetroScreenAudioBridge()
    : CapacitySamples(0)
    , ReadCursor(0)
    , WriteCursor(0)
    , AvailableSamples(0)
    , UnderrunCount(0)
    , OverrunCount(0)
    , TotalSamplesPushed(0)
    , TotalSamplesPopped(0)
{
}

void FRetroScreenAudioBridge::Initialize(int32 InCapacitySamples)
{
    const int32 NewCapacity = FMath::Max(InCapacitySamples, 2);

    FScopeLock Lock(&BufferMutex);

    CapacitySamples = NewCapacity;
    Buffer.SetNumZeroed(CapacitySamples);
    ReadCursor = 0;
    WriteCursor = 0;
    AvailableSamples = 0;
    UnderrunCount = 0;
    OverrunCount = 0;
    TotalSamplesPushed = 0;
    TotalSamplesPopped = 0;
}

void FRetroScreenAudioBridge::Reset()
{
    FScopeLock Lock(&BufferMutex);

    CapacitySamples = 0;
    Buffer.Reset();
    ReadCursor = 0;
    WriteCursor = 0;
    AvailableSamples = 0;
    UnderrunCount = 0;
    OverrunCount = 0;
    TotalSamplesPushed = 0;
    TotalSamplesPopped = 0;
}

int32 FRetroScreenAudioBridge::PushInterleavedSamples(const float* InSamples, int32 NumSamples)
{
    if (InSamples == nullptr || NumSamples <= 0)
    {
        return 0;
    }

    FScopeLock Lock(&BufferMutex);

    if (CapacitySamples <= 0)
    {
        return 0;
    }

    const int32 FreeSamples = CapacitySamples - AvailableSamples;
    const int32 SamplesToWrite = FMath::Min(NumSamples, FreeSamples);

    if (SamplesToWrite < NumSamples)
    {
        OverrunCount += static_cast<uint64>(NumSamples - SamplesToWrite);
    }

    // Two-chunk Memcpy: write up to the end of the ring buffer, then wrap to the start.
    const int32 FirstChunk = FMath::Min(SamplesToWrite, CapacitySamples - WriteCursor);
    FMemory::Memcpy(Buffer.GetData() + WriteCursor, InSamples, sizeof(float) * FirstChunk);
    if (SamplesToWrite > FirstChunk)
    {
        FMemory::Memcpy(Buffer.GetData(), InSamples + FirstChunk, sizeof(float) * (SamplesToWrite - FirstChunk));
    }
    WriteCursor = (WriteCursor + SamplesToWrite) % CapacitySamples;

    AvailableSamples += SamplesToWrite;
    TotalSamplesPushed += static_cast<uint64>(SamplesToWrite);
    return SamplesToWrite;
}

int32 FRetroScreenAudioBridge::PopInterleavedSamples(float* OutSamples, int32 NumSamples)
{
    if (OutSamples == nullptr || NumSamples <= 0)
    {
        return 0;
    }

    FScopeLock Lock(&BufferMutex);

    if (CapacitySamples <= 0)
    {
        FMemory::Memzero(OutSamples, sizeof(float) * NumSamples);
        UnderrunCount += static_cast<uint64>(NumSamples);
        return 0;
    }

    const int32 SamplesToRead = FMath::Min(NumSamples, AvailableSamples);

    // Two-chunk Memcpy: read up to the end of the ring buffer, then wrap to the start.
    const int32 FirstChunk = FMath::Min(SamplesToRead, CapacitySamples - ReadCursor);
    FMemory::Memcpy(OutSamples, Buffer.GetData() + ReadCursor, sizeof(float) * FirstChunk);
    if (SamplesToRead > FirstChunk)
    {
        FMemory::Memcpy(OutSamples + FirstChunk, Buffer.GetData(), sizeof(float) * (SamplesToRead - FirstChunk));
    }
    ReadCursor = (ReadCursor + SamplesToRead) % CapacitySamples;

    AvailableSamples -= SamplesToRead;
    TotalSamplesPopped += static_cast<uint64>(SamplesToRead);

    if (SamplesToRead < NumSamples)
    {
        const int32 MissingSamples = NumSamples - SamplesToRead;
        FMemory::Memzero(OutSamples + SamplesToRead, sizeof(float) * MissingSamples);
        UnderrunCount += static_cast<uint64>(MissingSamples);
    }

    return SamplesToRead;
}

int32 FRetroScreenAudioBridge::GetAvailableSamples() const
{
    FScopeLock Lock(&BufferMutex);
    return AvailableSamples;
}
