#include "RetroScreenEmulatorWorker.h"

#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"

FRetroScreenEmulatorWorker::FRetroScreenEmulatorWorker(float InTargetFps)
    : bStopRequested(false)
    , bPauseRequested(false)
    , TargetFrameDeltaSeconds(1.0 / FMath::Max<double>(1.0, static_cast<double>(InTargetFps)))
    , FrameDurationAccumulatedMs(0.0)
{
}

void FRetroScreenEmulatorWorker::SetFrameStepCallback(TFunction<void()> InCallback)
{
    FScopeLock Lock(&CallbackMutex);
    FrameStepCallback = MoveTemp(InCallback);
}

FRetroScreenEmulatorWorkerStats FRetroScreenEmulatorWorker::GetStats() const
{
    FScopeLock Lock(&StatsMutex);
    return Stats;
}

uint32 FRetroScreenEmulatorWorker::Run()
{
    // Cache the callback once — SetFrameStepCallback must not be called after Run() starts.
    TFunction<void()> CachedCallback;
    {
        FScopeLock CallbackLock(&CallbackMutex);
        CachedCallback = FrameStepCallback;
    }

    double NextFrameSeconds = FPlatformTime::Seconds();

    while (!bStopRequested)
    {
        if (bPauseRequested)
        {
            FPlatformProcess::SleepNoStats(0.01f);
            NextFrameSeconds = FPlatformTime::Seconds();
            continue;
        }

        const double FrameStartSeconds = FPlatformTime::Seconds();
        if (CachedCallback)
        {
            CachedCallback();
        }

        const float FrameDurationMs = static_cast<float>((FPlatformTime::Seconds() - FrameStartSeconds) * 1000.0);
        {
            FScopeLock StatsLock(&StatsMutex);
            Stats.TotalFramesExecuted += 1;
            Stats.LastFrameDurationMs = FrameDurationMs;
            FrameDurationAccumulatedMs += FrameDurationMs;
            Stats.AverageFrameDurationMs = static_cast<float>(FrameDurationAccumulatedMs / FMath::Max<int64>(1, Stats.TotalFramesExecuted));
            Stats.MaxFrameDurationMs = FMath::Max(Stats.MaxFrameDurationMs, FrameDurationMs);
            Stats.LastFrameStartSeconds = static_cast<float>(FrameStartSeconds);
        }

        NextFrameSeconds += TargetFrameDeltaSeconds;
        const double SleepSeconds = NextFrameSeconds - FPlatformTime::Seconds();
        if (SleepSeconds > 0.0)
        {
            FPlatformProcess::SleepNoStats(static_cast<float>(SleepSeconds));
            continue;
        }

        if ((FPlatformTime::Seconds() - NextFrameSeconds) > TargetFrameDeltaSeconds)
        {
            NextFrameSeconds = FPlatformTime::Seconds();
        }
    }

    return 0;
}

void FRetroScreenEmulatorWorker::Stop()
{
    bStopRequested = true;
}

void FRetroScreenEmulatorWorker::SetPaused(bool bPaused)
{
    bPauseRequested = bPaused;
}
