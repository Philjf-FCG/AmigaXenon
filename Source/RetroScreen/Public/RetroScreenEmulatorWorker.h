#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"

struct FRetroScreenEmulatorWorkerStats
{
    int64 TotalFramesExecuted = 0;
    float LastFrameDurationMs = 0.0f;
    float AverageFrameDurationMs = 0.0f;
    float MaxFrameDurationMs = 0.0f;
    float LastFrameStartSeconds = 0.0f;
};

class FRetroScreenEmulatorWorker final : public FRunnable
{
public:
    explicit FRetroScreenEmulatorWorker(float InTargetFps = 50.0f);
    virtual ~FRetroScreenEmulatorWorker() override = default;

    void SetFrameStepCallback(TFunction<void()> InCallback);
    FRetroScreenEmulatorWorkerStats GetStats() const;
    void SetPaused(bool bPaused);
    bool IsPaused() const { return bPauseRequested; }

    virtual uint32 Run() override;
    virtual void Stop() override;

private:
    FThreadSafeBool bStopRequested;
    FThreadSafeBool bPauseRequested;
    double TargetFrameDeltaSeconds;

    mutable FCriticalSection CallbackMutex;
    TFunction<void()> FrameStepCallback;

    mutable FCriticalSection StatsMutex;
    FRetroScreenEmulatorWorkerStats Stats;
    double FrameDurationAccumulatedMs;
};
