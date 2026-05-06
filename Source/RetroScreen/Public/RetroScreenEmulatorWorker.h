#pragma once

#include "HAL/Runnable.h"

class FRetroScreenEmulatorWorker final : public FRunnable
{
public:
    FRetroScreenEmulatorWorker();
    virtual ~FRetroScreenEmulatorWorker() override = default;

    virtual uint32 Run() override;
    virtual void Stop() override;

private:
    FThreadSafeBool bStopRequested;
};
