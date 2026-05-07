#include "RetroScreenRemapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "RetroScreenManager.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

namespace
{
static const TCHAR* ButtonIdToName(ERetroJoypadButton Id)
{
    switch (Id)
    {
    case ERetroJoypadButton::B:      return TEXT("B (Fire)");
    case ERetroJoypadButton::Y:      return TEXT("Y");
    case ERetroJoypadButton::Select: return TEXT("Select");
    case ERetroJoypadButton::Start:  return TEXT("Start");
    case ERetroJoypadButton::Up:     return TEXT("Up");
    case ERetroJoypadButton::Down:   return TEXT("Down");
    case ERetroJoypadButton::Left:   return TEXT("Left");
    case ERetroJoypadButton::Right:  return TEXT("Right");
    case ERetroJoypadButton::A:      return TEXT("A");
    case ERetroJoypadButton::X:      return TEXT("X");
    case ERetroJoypadButton::L:      return TEXT("L");
    case ERetroJoypadButton::R:      return TEXT("R");
    case ERetroJoypadButton::L2:     return TEXT("L2");
    case ERetroJoypadButton::R2:     return TEXT("R2");
    case ERetroJoypadButton::L3:     return TEXT("L3");
    case ERetroJoypadButton::R3:     return TEXT("R3");
    default:                         return TEXT("?");
    }
}

USpinBox* MakeButtonSpinBox(UWidgetTree* WidgetTree, UVerticalBox* Parent,
                            const FString& Label, ERetroJoypadButton DefaultId)
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

    // Label column
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

    // SpinBox for the button id (0–15)
    USpinBox* Spin = WidgetTree->ConstructWidget<USpinBox>();
    if (Spin != nullptr)
    {
        Spin->SetMinValue(0);
        Spin->SetMaxValue(15);
        Spin->SetMinSliderValue(0);
        Spin->SetMaxSliderValue(15);
        Spin->SetValue(static_cast<float>(DefaultId));
        Spin->SetDelta(1.0f);
        if (UHorizontalBoxSlot* SpinSlot = Row->AddChildToHorizontalBox(Spin))
        {
            SpinSlot->SetPadding(FMargin(0.0f));
            SpinSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        }
    }

    return Spin;
}
} // namespace

// ---------------------------------------------------------------------------

void URetroScreenRemapWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildFallbackLayoutIfNeeded();

    // Populate controls from the manager's current profile if available
    if (ManagerPtr != nullptr)
    {
        RefreshControlsFromProfile(ManagerPtr->GetButtonProfile());
    }
}

void URetroScreenRemapWidget::SetRetroScreenManager(ARetroScreenManager* InManager)
{
    ManagerPtr = InManager;
    if (ManagerPtr != nullptr)
    {
        RefreshControlsFromProfile(ManagerPtr->GetButtonProfile());
    }
}

void URetroScreenRemapWidget::ApplyAndSave()
{
    if (ManagerPtr == nullptr)
    {
        return;
    }

    const FRetroScreenButtonProfile Profile = ReadProfileFromControls();
    ManagerPtr->SetButtonProfile(Profile, /*bSaveToDisk=*/true);
}

void URetroScreenRemapWidget::ResetToDefaults()
{
    FRetroScreenButtonProfile Defaults;
    Defaults.ResetToDefaults();
    RefreshControlsFromProfile(Defaults);
}

void URetroScreenRemapWidget::HandleApplyClicked()
{
    ApplyAndSave();
}

void URetroScreenRemapWidget::HandleResetClicked()
{
    ResetToDefaults();
}

void URetroScreenRemapWidget::HandleCloseClicked()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------
// Fallback layout
// ---------------------------------------------------------------------------

void URetroScreenRemapWidget::BuildFallbackLayoutIfNeeded()
{
    if (bFallbackBuilt)
    {
        return;
    }

    // If a Blueprint already bound any widget, honour it
    if (FireSpinBox != nullptr || ApplyButton != nullptr)
    {
        bFallbackBuilt = true;
        return;
    }

    if (WidgetTree == nullptr)
    {
        return;
    }

    UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>();
    if (VBox == nullptr)
    {
        return;
    }
    WidgetTree->RootWidget = VBox;

    // Title
    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
    if (Title != nullptr)
    {
        Title->SetText(FText::FromString(TEXT("Button Remapping  (enter libretro id 0-15)")));
        if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(Title))
        {
            S->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
        }
    }

    FireSpinBox  = MakeButtonSpinBox(WidgetTree, VBox, TEXT("Fire  (default B=0)"),  ERetroJoypadButton::B);
    UpSpinBox    = MakeButtonSpinBox(WidgetTree, VBox, TEXT("Up    (default 4)"),     ERetroJoypadButton::Up);
    DownSpinBox  = MakeButtonSpinBox(WidgetTree, VBox, TEXT("Down  (default 5)"),     ERetroJoypadButton::Down);
    LeftSpinBox  = MakeButtonSpinBox(WidgetTree, VBox, TEXT("Left  (default 6)"),     ERetroJoypadButton::Left);
    RightSpinBox = MakeButtonSpinBox(WidgetTree, VBox, TEXT("Right (default 7)"),     ERetroJoypadButton::Right);

    // Button row
    UHorizontalBox* BtnRow = WidgetTree->ConstructWidget<UHorizontalBox>();
    if (BtnRow != nullptr)
    {
        if (UVerticalBoxSlot* BRS = VBox->AddChildToVerticalBox(BtnRow))
        {
            BRS->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
        }

        auto AddBtn = [&](TObjectPtr<UButton>& OutRef, const FString& Label) -> UButton*
        {
            UButton* Btn = WidgetTree->ConstructWidget<UButton>();
            if (Btn == nullptr)
            {
                return nullptr;
            }
            UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
            if (BtnLabel != nullptr)
            {
                BtnLabel->SetText(FText::FromString(Label));
                Btn->AddChild(BtnLabel);
            }
            if (UHorizontalBoxSlot* HS = BtnRow->AddChildToHorizontalBox(Btn))
            {
                HS->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
            }
            OutRef = Btn;
            return Btn;
        };

        AddBtn(ApplyButton, TEXT("Apply & Save"));
        AddBtn(ResetButton, TEXT("Reset Defaults"));
        AddBtn(CloseButton, TEXT("Close"));
    }

    // Wire button delegates
    if (ApplyButton != nullptr)
    {
        ApplyButton->OnClicked.AddDynamic(this, &URetroScreenRemapWidget::HandleApplyClicked);
    }
    if (ResetButton != nullptr)
    {
        ResetButton->OnClicked.AddDynamic(this, &URetroScreenRemapWidget::HandleResetClicked);
    }
    if (CloseButton != nullptr)
    {
        CloseButton->OnClicked.AddDynamic(this, &URetroScreenRemapWidget::HandleCloseClicked);
    }

    bFallbackBuilt = true;
}

// ---------------------------------------------------------------------------

void URetroScreenRemapWidget::RefreshControlsFromProfile(const FRetroScreenButtonProfile& Profile)
{
    if (FireSpinBox  != nullptr) { FireSpinBox->SetValue(static_cast<float>(Profile.FireButton));  }
    if (UpSpinBox    != nullptr) { UpSpinBox->SetValue(static_cast<float>(Profile.UpButton));      }
    if (DownSpinBox  != nullptr) { DownSpinBox->SetValue(static_cast<float>(Profile.DownButton));  }
    if (LeftSpinBox  != nullptr) { LeftSpinBox->SetValue(static_cast<float>(Profile.LeftButton));  }
    if (RightSpinBox != nullptr) { RightSpinBox->SetValue(static_cast<float>(Profile.RightButton));}
}

FRetroScreenButtonProfile URetroScreenRemapWidget::ReadProfileFromControls() const
{
    FRetroScreenButtonProfile Profile;

    auto ReadSpin = [](USpinBox* Spin, ERetroJoypadButton& OutId)
    {
        if (Spin == nullptr)
        {
            return;
        }
        const int32 Clamped = FMath::Clamp(FMath::RoundToInt(Spin->GetValue()), 0, 15);
        OutId = static_cast<ERetroJoypadButton>(Clamped);
    };

    ReadSpin(FireSpinBox,  Profile.FireButton);
    ReadSpin(UpSpinBox,    Profile.UpButton);
    ReadSpin(DownSpinBox,  Profile.DownButton);
    ReadSpin(LeftSpinBox,  Profile.LeftButton);
    ReadSpin(RightSpinBox, Profile.RightButton);

    return Profile;
}
