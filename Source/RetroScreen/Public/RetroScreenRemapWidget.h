#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"

#include "RetroScreenRemapWidget.generated.h"

class ARetroScreenManager;
class UButton;
class USpinBox;
class UTextBlock;
class UVerticalBox;

/**
 * Libretro joypad button IDs (RETRO_DEVICE_ID_JOYPAD_*).
 * Mirrors the libretro.h defines so Blueprint / UMG code can reference them
 * without including the C header directly.
 */
UENUM(BlueprintType)
enum class ERetroJoypadButton : uint8
{
    B          = 0  UMETA(DisplayName = "B (Fire)"),
    Y          = 1  UMETA(DisplayName = "Y"),
    Select     = 2  UMETA(DisplayName = "Select"),
    Start      = 3  UMETA(DisplayName = "Start"),
    Up         = 4  UMETA(DisplayName = "Up"),
    Down       = 5  UMETA(DisplayName = "Down"),
    Left       = 6  UMETA(DisplayName = "Left"),
    Right      = 7  UMETA(DisplayName = "Right"),
    A          = 8  UMETA(DisplayName = "A"),
    X          = 9  UMETA(DisplayName = "X"),
    L          = 10 UMETA(DisplayName = "L"),
    R          = 11 UMETA(DisplayName = "R"),
    L2         = 12 UMETA(DisplayName = "L2"),
    R2         = 13 UMETA(DisplayName = "R2"),
    L3         = 14 UMETA(DisplayName = "L3"),
    R3         = 15 UMETA(DisplayName = "R3"),
};

/**
 * Stores the mapping from each RetroScreen virtual action to a libretro
 * joypad button id.  Default values reflect the standard Amiga single-button
 * joystick layout expected by PUAE.
 */
USTRUCT(BlueprintType)
struct RETROSCREEN_API FRetroScreenButtonProfile
{
    GENERATED_BODY()

    /** Gamepad button sent for the PrimaryFire action. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|Remap")
    ERetroJoypadButton FireButton = ERetroJoypadButton::B;

    /** Gamepad button sent for the MoveUp action (d-pad fallback). */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|Remap")
    ERetroJoypadButton UpButton = ERetroJoypadButton::Up;

    /** Gamepad button sent for the MoveDown action. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|Remap")
    ERetroJoypadButton DownButton = ERetroJoypadButton::Down;

    /** Gamepad button sent for the MoveLeft action. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|Remap")
    ERetroJoypadButton LeftButton = ERetroJoypadButton::Left;

    /** Gamepad button sent for the MoveRight action. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RetroScreen|Remap")
    ERetroJoypadButton RightButton = ERetroJoypadButton::Right;

    /** Reset every mapping to the default Amiga layout. */
    void ResetToDefaults()
    {
        FireButton  = ERetroJoypadButton::B;
        UpButton    = ERetroJoypadButton::Up;
        DownButton  = ERetroJoypadButton::Down;
        LeftButton  = ERetroJoypadButton::Left;
        RightButton = ERetroJoypadButton::Right;
    }
};

/**
 * URetroScreenRemapWidget
 *
 * Displays the current button profile and lets the player reassign each
 * virtual action to a different libretro joypad button.  Changes are applied
 * immediately and saved to Saved/RetroScreen.ini under [RetroScreenButtonMap].
 *
 * A Blueprint subclass (WBP_RetroScreenRemap) can replace the layout via
 * BindWidgetOptional; the C++ fallback uses SpinBoxes so the feature works
 * without any Blueprint content.
 */
UCLASS(BlueprintType, Blueprintable)
class RETROSCREEN_API URetroScreenRemapWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** Populate controls from the profile currently active on the manager. */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Remap")
    void SetRetroScreenManager(ARetroScreenManager* InManager);

    /** Read back controls and push the updated profile to the manager + disk. */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Remap")
    void ApplyAndSave();

    /** Reset all controls to the default Amiga layout without saving. */
    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Remap")
    void ResetToDefaults();

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<USpinBox> FireSpinBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<USpinBox> UpSpinBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<USpinBox> DownSpinBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<USpinBox> LeftSpinBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<USpinBox> RightSpinBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> ApplyButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> ResetButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> CloseButton;

private:
    UFUNCTION()
    void HandleApplyClicked();

    UFUNCTION()
    void HandleResetClicked();

    UFUNCTION()
    void HandleCloseClicked();

    void BuildFallbackLayoutIfNeeded();
    void RefreshControlsFromProfile(const FRetroScreenButtonProfile& Profile);
    FRetroScreenButtonProfile ReadProfileFromControls() const;

    TObjectPtr<ARetroScreenManager> ManagerPtr;
    bool bFallbackBuilt = false;
};
