#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "RetroScreenGameMode.generated.h"

/**
 * ARetroScreenGameMode — default game mode for the AmigaXenon project.
 *
 * On PIE / game start it spawns the required runtime actors if they are not
 * already present in the level:
 *   - ARetroScreenManager  (emulator orchestrator)
 *   - AArcadeRoomActor     (environment shell)
 *
 * ARetroScreenManager with bAutoSpawnArcadeCabinet=true takes care of
 * spawning AArcadeCabinetActor and pointing the player camera at the screen.
 */
UCLASS(BlueprintType, Blueprintable)
class AMIGAXENON_API ARetroScreenGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ARetroScreenGameMode();

protected:
    virtual void StartPlay() override;
};
