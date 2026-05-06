#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RetroScreenManager.h"

#include "RetroScreenPauseMenuWidget.generated.h"

class APlayerController;
class UButton;
class UCheckBox;
class USlider;
class USpinBox;

UCLASS(BlueprintType, Blueprintable)
class RETROSCREEN_API URetroScreenPauseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|PauseMenu")
    void SetRetroScreenManager(ARetroScreenManager* InManager);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|PauseMenu")
    ARetroScreenManager* ResolveRetroScreenManager();

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|PauseMenu")
    FRetroScreenPauseMenuSettings GetCurrentSettings();

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|PauseMenu")
    void ApplySettings(const FRetroScreenPauseMenuSettings& NewSettings, bool bSaveToDisk = true);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|PauseMenu")
    void ResumeGame();

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|PauseMenu")
    void ReturnToEnvironment();

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|PauseMenu")
    void OpenInputMappingFlow();

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|PauseMenu")
    void QuitToDesktop();

private:
    UFUNCTION()
    void HandleResumeClicked();

    UFUNCTION()
    void HandleReturnClicked();

    UFUNCTION()
    void HandleInputMappingClicked();

    UFUNCTION()
    void HandleQuitClicked();

    UFUNCTION()
    void HandleAudioVolumeChanged(float NewValue);

    UFUNCTION()
    void HandleCrtStateChanged(bool bChecked);

    UFUNCTION()
    void HandleAxesToDpadChanged(bool bChecked);

    UFUNCTION()
    void HandleInvertYChanged(bool bChecked);

    UFUNCTION()
    void HandleJoypadPortChanged(float NewValue);

    UFUNCTION()
    void HandleJoypadDeadzoneChanged(float NewValue);

    void BuildFallbackLayoutIfNeeded();
    void RefreshControlsFromSettings();
    void ApplyControlsToSettings(bool bSaveToDisk);

    APlayerController* GetOwningPlayerController() const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|PauseMenu", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<ARetroScreenManager> RetroScreenManager;

    UPROPERTY(Transient)
    TObjectPtr<USlider> AudioVolumeSlider;

    UPROPERTY(Transient)
    TObjectPtr<UCheckBox> EnableCrtCheckBox;

    UPROPERTY(Transient)
    TObjectPtr<USpinBox> JoypadPortSpinBox;

    UPROPERTY(Transient)
    TObjectPtr<USpinBox> JoypadDeadzoneSpinBox;

    UPROPERTY(Transient)
    TObjectPtr<UCheckBox> AxesToDpadCheckBox;

    UPROPERTY(Transient)
    TObjectPtr<UCheckBox> InvertYCheckBox;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ResumeButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> InputMappingButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> ReturnButton;

    UPROPERTY(Transient)
    TObjectPtr<UButton> QuitButton;

    bool bFallbackLayoutBuilt = false;
    bool bRefreshingControls = false;
};
