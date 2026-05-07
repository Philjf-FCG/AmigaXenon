#include "RetroScreenVideoBridge.h"

FRetroScreenVideoBridge::FRetroScreenVideoBridge()
    : LatestBufferIndex(0)
    , bHasFrame(false)
    , NextSequence(1)
{
}

uint64 FRetroScreenVideoBridge::PublishFrame(const uint8* InPixels, int32 InWidth, int32 InHeight, int32 InPitch)
{
    if (InPixels == nullptr || InWidth <= 0 || InHeight <= 0 || InPitch <= 0)
    {
        return 0;
    }

    const int64 SourceSize = static_cast<int64>(InPitch) * static_cast<int64>(InHeight);
    if (SourceSize <= 0)
    {
        return 0;
    }

    FScopeLock Lock(&BufferMutex);

    const int32 WriteIndex = (LatestBufferIndex + 1) % 2;
    FRetroScreenVideoFrame& WriteBuffer = Buffers[WriteIndex];

    WriteBuffer.Width = InWidth;
    WriteBuffer.Height = InHeight;
    WriteBuffer.Pitch = InPitch;
    WriteBuffer.Sequence = NextSequence++;
    WriteBuffer.Pixels.SetNumUninitialized(static_cast<int32>(SourceSize));
    FMemory::Memcpy(WriteBuffer.Pixels.GetData(), InPixels, static_cast<SIZE_T>(SourceSize));

    LatestBufferIndex = WriteIndex;
    bHasFrame = true;
    return WriteBuffer.Sequence;
}

bool FRetroScreenVideoBridge::ConsumeLatestFrame(FRetroScreenVideoFrame& OutFrame)
{
    FScopeLock Lock(&BufferMutex);

    if (!bHasFrame)
    {
        return false;
    }

    // Clear the flag before copying so the game thread never re-uploads the same frame.
    // PublishFrame always writes the opposite slot so the pixel data remains valid here.
    bHasFrame = false;
    OutFrame = Buffers[LatestBufferIndex];
    return OutFrame.IsValid();
}

void FRetroScreenVideoBridge::Reset()
{
    FScopeLock Lock(&BufferMutex);

    Buffers[0] = FRetroScreenVideoFrame();
    Buffers[1] = FRetroScreenVideoFrame();
    LatestBufferIndex = 0;
    bHasFrame = false;
    NextSequence = 1;
}
