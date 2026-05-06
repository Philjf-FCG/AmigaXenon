#include "RetroScreenEmulatorWorker.h"

#include "HAL/PlatformProcess.h"

FRetroScreenEmulatorWorker::FRetroScreenEmulatorWorker()
    : bStopRequested(false)
{
}

uint32 FRetroScreenEmulatorWorker::Run()
{
    while (!bStopRequested)
    {
        // Placeholder for retro_run() frame execution on dedicated emulator thread.
        FPlatformProcess::SleepNoStats(1.0f / 60.0f);
    }

    return 0;
}

void FRetroScreenEmulatorWorker::Stop()
{
    bStopRequested = true;
}
