#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "RetroScreenProfilingSubsystem.generated.h"

class ARetroScreenManager;

UCLASS()
class RETROSCREEN_API URetroScreenProfilingSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

private:
    void StartAutoProfiling(UWorld& InWorld);
    void FinishAutoProfiling();

    TWeakObjectPtr<ARetroScreenManager> ActiveManager;
    bool bAutoProfilingActive = false;
};
