#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "RetroScreenSetupWidget.generated.h"

class ARetroScreenManager;
class UButton;
class UTextBlock;
class UVerticalBox;

/**
 * Issues detected during the first-run configuration check.
 * Multiple flags may be set simultaneously.
 */
USTRUCT(BlueprintType)
struct RETROSCREEN_API FRetroScreenSetupIssues
{
    GENERATED_BODY()

    /** The libretro core DLL is missing or the path is empty. */
    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Setup")
    bool bCoreMissing = false;

    /** The game disk (ADF/ROM) path is empty or the file does not exist. */
    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Setup")
    bool bGameDiskMissing = false;

    /** No Kickstart ROM was found in EmulatorData/Kickstart/. */
    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Setup")
    bool bKickstartMissing = false;

    /** Returns true when every required file is present. */
    bool IsComplete() const { return !bCoreMissing && !bGameDiskMissing && !bKickstartMissing; }
};

/**
 * URetroScreenSetupWidget
 *
 * Displayed on first launch (or whenever the emulator cannot start due to
 * missing files).  Shows a plain-text checklist of what needs to be provided
 * and the legal steps required to obtain Kickstart ROMs.
 *
 * A Blueprint subclass can fully customise the appearance; C++ provides a
 * self-contained fallback layout via NativeConstruct so no Blueprint is
 * required during early development.
 */
UCLASS(BlueprintType, Blueprintable)
class RETROSCREEN_API URetroScreenSetupWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** Populate the widget with the detected issues and refresh labels. */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Setup")
    void SetSetupIssues(const FRetroScreenSetupIssues& Issues);

    /** Allow the widget to reach back to the manager (e.g. to dismiss itself). */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Setup")
    void SetRetroScreenManager(ARetroScreenManager* InManager);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "RetroScreen|Setup")
    FRetroScreenSetupIssues CurrentIssues;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CoreStatusText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> GameDiskStatusText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> KickstartStatusText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> InstructionsText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> DismissButton;

private:
    UFUNCTION()
    void HandleDismissClicked();

    void BuildFallbackLayoutIfNeeded();
    void RefreshStatusLabels();

    TObjectPtr<ARetroScreenManager> ManagerPtr;
    bool bFallbackBuilt = false;
};
