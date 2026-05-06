#include "RetroScreenPauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
UHorizontalBox* CreateLabeledRow(UWidgetTree* WidgetTree, UVerticalBox* Parent, const FString& Label)
{
    if (WidgetTree == nullptr || Parent == nullptr)
    {
        return nullptr;
    }

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    if (Row == nullptr)
    {
        return nullptr;
    }

    if (UVerticalBoxSlot* RowSlot = Parent->AddChildToVerticalBox(Row))
    {
        RowSlot->SetPadding(FMargin(0.0f, 4.0f));
    }

    UTextBlock* LabelText = WidgetTree->ConstructWidget<UTextBlock>();
    if (LabelText != nullptr)
    {
        LabelText->SetText(FText::FromString(Label));
        if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(LabelText))
        {
            LabelSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
            LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        }
    }

    return Row;
}

void AddButtonLabel(UWidgetTree* WidgetTree, UButton* Button, const FString& Label)
{
    if (WidgetTree == nullptr || Button == nullptr)
    {
        return;
    }

    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>();
    if (ButtonText != nullptr)
    {
        ButtonText->SetText(FText::FromString(Label));
        Button->AddChild(ButtonText);
    }
}
}

void URetroScreenPauseMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BuildFallbackLayoutIfNeeded();
    RefreshControlsFromSettings();
}

void URetroScreenPauseMenuWidget::SetRetroScreenManager(ARetroScreenManager* InManager)
{
    RetroScreenManager = InManager;
}

ARetroScreenManager* URetroScreenPauseMenuWidget::ResolveRetroScreenManager()
{
    if (RetroScreenManager != nullptr)
    {
        return RetroScreenManager;
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return nullptr;
    }

    for (TActorIterator<ARetroScreenManager> It(World); It; ++It)
    {
        RetroScreenManager = *It;
        break;
    }

    return RetroScreenManager;
}

FRetroScreenPauseMenuSettings URetroScreenPauseMenuWidget::GetCurrentSettings()
{
    if (ARetroScreenManager* Manager = ResolveRetroScreenManager())
    {
        return Manager->GetPauseMenuSettings();
    }

    return FRetroScreenPauseMenuSettings();
}

void URetroScreenPauseMenuWidget::ApplySettings(const FRetroScreenPauseMenuSettings& NewSettings, bool bSaveToDisk)
{
    if (ARetroScreenManager* Manager = ResolveRetroScreenManager())
    {
        Manager->ApplyPauseMenuSettings(NewSettings, bSaveToDisk);
    }
}

void URetroScreenPauseMenuWidget::HandleResumeClicked()
{
    ResumeGame();
}

void URetroScreenPauseMenuWidget::HandleReturnClicked()
{
    ReturnToEnvironment();
}

void URetroScreenPauseMenuWidget::HandleInputMappingClicked()
{
    OpenInputMappingFlow();
}

void URetroScreenPauseMenuWidget::HandleQuitClicked()
{
    QuitToDesktop();
}

void URetroScreenPauseMenuWidget::HandleAudioVolumeChanged(float NewValue)
{
    if (bRefreshingControls)
    {
        return;
    }

    ApplyControlsToSettings(true);
}

void URetroScreenPauseMenuWidget::HandleCrtStateChanged(bool bChecked)
{
    if (bRefreshingControls)
    {
        return;
    }

    ApplyControlsToSettings(true);
}

void URetroScreenPauseMenuWidget::HandleAxesToDpadChanged(bool bChecked)
{
    if (bRefreshingControls)
    {
        return;
    }

    ApplyControlsToSettings(true);
}

void URetroScreenPauseMenuWidget::HandleInvertYChanged(bool bChecked)
{
    if (bRefreshingControls)
    {
        return;
    }

    ApplyControlsToSettings(true);
}

void URetroScreenPauseMenuWidget::HandleJoypadPortChanged(float NewValue)
{
    if (bRefreshingControls)
    {
        return;
    }

    ApplyControlsToSettings(true);
}

void URetroScreenPauseMenuWidget::HandleJoypadDeadzoneChanged(float NewValue)
{
    if (bRefreshingControls)
    {
        return;
    }

    ApplyControlsToSettings(true);
}

void URetroScreenPauseMenuWidget::BuildFallbackLayoutIfNeeded()
{
    if (bFallbackLayoutBuilt || WidgetTree == nullptr)
    {
        return;
    }

    if (WidgetTree->RootWidget != nullptr)
    {
        bFallbackLayoutBuilt = true;
        return;
    }

    UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>();
    WidgetTree->RootWidget = RootBox;

    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
    Title->SetText(FText::FromString(TEXT("RetroScreen Pause")));
    if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(Title))
    {
        TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
    }

    if (UHorizontalBox* VolumeRow = CreateLabeledRow(WidgetTree, RootBox, TEXT("Audio Volume")))
    {
        AudioVolumeSlider = WidgetTree->ConstructWidget<USlider>();
        if (AudioVolumeSlider != nullptr)
        {
            AudioVolumeSlider->SetMinValue(0.0f);
            AudioVolumeSlider->SetMaxValue(2.0f);
            AudioVolumeSlider->SetStepSize(0.01f);
            AudioVolumeSlider->OnValueChanged.AddDynamic(this, &URetroScreenPauseMenuWidget::HandleAudioVolumeChanged);

            if (UHorizontalBoxSlot* VolumeSlot = VolumeRow->AddChildToHorizontalBox(AudioVolumeSlider))
            {
                VolumeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            }
        }
    }

    if (UHorizontalBox* CrtRow = CreateLabeledRow(WidgetTree, RootBox, TEXT("Enable CRT")))
    {
        EnableCrtCheckBox = WidgetTree->ConstructWidget<UCheckBox>();
        if (EnableCrtCheckBox != nullptr)
        {
            EnableCrtCheckBox->OnCheckStateChanged.AddDynamic(this, &URetroScreenPauseMenuWidget::HandleCrtStateChanged);
            CrtRow->AddChildToHorizontalBox(EnableCrtCheckBox);
        }
    }

    if (UHorizontalBox* PortRow = CreateLabeledRow(WidgetTree, RootBox, TEXT("Joypad Port")))
    {
        JoypadPortSpinBox = WidgetTree->ConstructWidget<USpinBox>();
        if (JoypadPortSpinBox != nullptr)
        {
            JoypadPortSpinBox->SetMinValue(0.0f);
            JoypadPortSpinBox->SetMaxValue(8.0f);
            JoypadPortSpinBox->SetDelta(1.0f);
            JoypadPortSpinBox->SetMinSliderValue(0.0f);
            JoypadPortSpinBox->SetMaxSliderValue(8.0f);
            JoypadPortSpinBox->OnValueChanged.AddDynamic(this, &URetroScreenPauseMenuWidget::HandleJoypadPortChanged);
            PortRow->AddChildToHorizontalBox(JoypadPortSpinBox);
        }
    }

    if (UHorizontalBox* DeadzoneRow = CreateLabeledRow(WidgetTree, RootBox, TEXT("Joypad Deadzone")))
    {
        JoypadDeadzoneSpinBox = WidgetTree->ConstructWidget<USpinBox>();
        if (JoypadDeadzoneSpinBox != nullptr)
        {
            JoypadDeadzoneSpinBox->SetMinValue(0.0f);
            JoypadDeadzoneSpinBox->SetMaxValue(32767.0f);
            JoypadDeadzoneSpinBox->SetDelta(128.0f);
            JoypadDeadzoneSpinBox->SetMinSliderValue(0.0f);
            JoypadDeadzoneSpinBox->SetMaxSliderValue(32767.0f);
            JoypadDeadzoneSpinBox->OnValueChanged.AddDynamic(this, &URetroScreenPauseMenuWidget::HandleJoypadDeadzoneChanged);
            DeadzoneRow->AddChildToHorizontalBox(JoypadDeadzoneSpinBox);
        }
    }

    if (UHorizontalBox* AxesRow = CreateLabeledRow(WidgetTree, RootBox, TEXT("Map Axes To D-Pad")))
    {
        AxesToDpadCheckBox = WidgetTree->ConstructWidget<UCheckBox>();
        if (AxesToDpadCheckBox != nullptr)
        {
            AxesToDpadCheckBox->OnCheckStateChanged.AddDynamic(this, &URetroScreenPauseMenuWidget::HandleAxesToDpadChanged);
            AxesRow->AddChildToHorizontalBox(AxesToDpadCheckBox);
        }
    }

    if (UHorizontalBox* InvertYRow = CreateLabeledRow(WidgetTree, RootBox, TEXT("Invert Y Axis")))
    {
        InvertYCheckBox = WidgetTree->ConstructWidget<UCheckBox>();
        if (InvertYCheckBox != nullptr)
        {
            InvertYCheckBox->OnCheckStateChanged.AddDynamic(this, &URetroScreenPauseMenuWidget::HandleInvertYChanged);
            InvertYRow->AddChildToHorizontalBox(InvertYCheckBox);
        }
    }

    ResumeButton = WidgetTree->ConstructWidget<UButton>();
    if (ResumeButton != nullptr)
    {
        ResumeButton->OnClicked.AddDynamic(this, &URetroScreenPauseMenuWidget::HandleResumeClicked);
        AddButtonLabel(WidgetTree, ResumeButton, TEXT("Resume"));
        if (UVerticalBoxSlot* ResumeButtonSlot = RootBox->AddChildToVerticalBox(ResumeButton))
        {
            ResumeButtonSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
        }
    }

    InputMappingButton = WidgetTree->ConstructWidget<UButton>();
    if (InputMappingButton != nullptr)
    {
        InputMappingButton->OnClicked.AddDynamic(this, &URetroScreenPauseMenuWidget::HandleInputMappingClicked);
        AddButtonLabel(WidgetTree, InputMappingButton, TEXT("Input Mapping"));
        if (UVerticalBoxSlot* InputMappingButtonSlot = RootBox->AddChildToVerticalBox(InputMappingButton))
        {
            InputMappingButtonSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
        }
    }

    ReturnButton = WidgetTree->ConstructWidget<UButton>();
    if (ReturnButton != nullptr)
    {
        ReturnButton->OnClicked.AddDynamic(this, &URetroScreenPauseMenuWidget::HandleReturnClicked);
        AddButtonLabel(WidgetTree, ReturnButton, TEXT("Return To Environment"));
        if (UVerticalBoxSlot* ReturnButtonSlot = RootBox->AddChildToVerticalBox(ReturnButton))
        {
            ReturnButtonSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
        }
    }

    QuitButton = WidgetTree->ConstructWidget<UButton>();
    if (QuitButton != nullptr)
    {
        QuitButton->OnClicked.AddDynamic(this, &URetroScreenPauseMenuWidget::HandleQuitClicked);
        AddButtonLabel(WidgetTree, QuitButton, TEXT("Quit"));
        if (UVerticalBoxSlot* QuitButtonSlot = RootBox->AddChildToVerticalBox(QuitButton))
        {
            QuitButtonSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
        }
    }

    bFallbackLayoutBuilt = true;
}

void URetroScreenPauseMenuWidget::RefreshControlsFromSettings()
{
    const FRetroScreenPauseMenuSettings Settings = GetCurrentSettings();

    bRefreshingControls = true;

    if (AudioVolumeSlider != nullptr)
    {
        AudioVolumeSlider->SetValue(Settings.AudioVolume);
    }

    if (EnableCrtCheckBox != nullptr)
    {
        EnableCrtCheckBox->SetIsChecked(Settings.bEnableCrt);
    }

    if (JoypadPortSpinBox != nullptr)
    {
        JoypadPortSpinBox->SetValue(static_cast<float>(Settings.JoypadPort));
    }

    if (JoypadDeadzoneSpinBox != nullptr)
    {
        JoypadDeadzoneSpinBox->SetValue(static_cast<float>(Settings.JoypadDeadzone));
    }

    if (AxesToDpadCheckBox != nullptr)
    {
        AxesToDpadCheckBox->SetIsChecked(Settings.bMapAxesToDpad);
    }

    if (InvertYCheckBox != nullptr)
    {
        InvertYCheckBox->SetIsChecked(Settings.bInvertY);
    }

    bRefreshingControls = false;
}

void URetroScreenPauseMenuWidget::ApplyControlsToSettings(bool bSaveToDisk)
{
    FRetroScreenPauseMenuSettings Settings = GetCurrentSettings();

    if (AudioVolumeSlider != nullptr)
    {
        Settings.AudioVolume = AudioVolumeSlider->GetValue();
    }

    if (EnableCrtCheckBox != nullptr)
    {
        Settings.bEnableCrt = EnableCrtCheckBox->IsChecked();
    }

    if (JoypadPortSpinBox != nullptr)
    {
        Settings.JoypadPort = FMath::RoundToInt(JoypadPortSpinBox->GetValue());
    }

    if (JoypadDeadzoneSpinBox != nullptr)
    {
        Settings.JoypadDeadzone = FMath::RoundToInt(JoypadDeadzoneSpinBox->GetValue());
    }

    if (AxesToDpadCheckBox != nullptr)
    {
        Settings.bMapAxesToDpad = AxesToDpadCheckBox->IsChecked();
    }

    if (InvertYCheckBox != nullptr)
    {
        Settings.bInvertY = InvertYCheckBox->IsChecked();
    }

    ApplySettings(Settings, bSaveToDisk);
}

void URetroScreenPauseMenuWidget::ResumeGame()
{
    if (ARetroScreenManager* Manager = ResolveRetroScreenManager())
    {
        Manager->ClosePauseMenu(true);
    }
}

void URetroScreenPauseMenuWidget::ReturnToEnvironment()
{
    if (ARetroScreenManager* Manager = ResolveRetroScreenManager())
    {
        Manager->ClosePauseMenu(false);
    }
}

void URetroScreenPauseMenuWidget::OpenInputMappingFlow()
{
    if (ARetroScreenManager* Manager = ResolveRetroScreenManager())
    {
        Manager->ClosePauseMenu(false);
    }
}

void URetroScreenPauseMenuWidget::QuitToDesktop()
{
    if (APlayerController* PlayerController = GetOwningPlayerController())
    {
        UKismetSystemLibrary::QuitGame(this, PlayerController, EQuitPreference::Quit, false);
    }
}

APlayerController* URetroScreenPauseMenuWidget::GetOwningPlayerController() const
{
    if (APlayerController* OwningController = GetOwningPlayer())
    {
        return OwningController;
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return nullptr;
    }

    return World->GetFirstPlayerController();
}
