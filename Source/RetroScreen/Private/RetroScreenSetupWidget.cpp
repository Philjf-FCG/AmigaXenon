#include "RetroScreenSetupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "RetroScreenManager.h"

// ---------------------------------------------------------------------------
// Legal instructions shown whenever a Kickstart ROM is missing.
// Kickstart ROMs are copyrighted by Cloanto/Hyperion.  Players must obtain
// them from legally-owned hardware or from licensed software such as the
// Amiga Forever package.
// ---------------------------------------------------------------------------
static const TCHAR* KickstartLegalText =
    TEXT("Kickstart ROM Setup (legal steps):\n"
         "  1. Own an original Amiga computer — the ROM is on-chip.\n"
         "  2. Use a ROM dumper tool (e.g. mkromchk) to extract the image legally.\n"
         "  OR\n"
         "  3. Purchase 'Amiga Forever' from Cloanto (amigaforever.com) which\n"
         "     includes licenced Kickstart images.\n\n"
         "Place the .rom file inside:\n"
         "  EmulatorData/Kickstart/\n\n"
         "Typical file names accepted by PUAE:\n"
         "  kick13.rom   (Kickstart 1.3 — Amiga 500)\n"
         "  kick20.rom   (Kickstart 2.0)\n"
         "  kick31.rom   (Kickstart 3.1)\n\n"
         "Restart the game after placing the file.");

// ---------------------------------------------------------------------------

void URetroScreenSetupWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildFallbackLayoutIfNeeded();
    RefreshStatusLabels();
}

void URetroScreenSetupWidget::SetSetupIssues(const FRetroScreenSetupIssues& Issues)
{
    CurrentIssues = Issues;
    RefreshStatusLabels();
}

void URetroScreenSetupWidget::SetRetroScreenManager(ARetroScreenManager* InManager)
{
    ManagerPtr = InManager;
}

void URetroScreenSetupWidget::HandleDismissClicked()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

// ---------------------------------------------------------------------------
// Fallback layout — built once when no Blueprint subclass provides widgets
// ---------------------------------------------------------------------------

void URetroScreenSetupWidget::BuildFallbackLayoutIfNeeded()
{
    if (bFallbackBuilt)
    {
        return;
    }

    // If a Blueprint already bound widgets, don't overwrite them.
    if (CoreStatusText != nullptr || DismissButton != nullptr)
    {
        bFallbackBuilt = true;
        return;
    }

    if (WidgetTree == nullptr)
    {
        return;
    }

    // Root scroll box so the instructions don't clip on small screens
    UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
    if (ScrollBox == nullptr)
    {
        return;
    }
    WidgetTree->RootWidget = ScrollBox;

    UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>();
    if (VBox == nullptr)
    {
        return;
    }
    if (UScrollBoxSlot* ScrollSlot = Cast<UScrollBoxSlot>(ScrollBox->AddChild(VBox)))
    {
        ScrollSlot->SetPadding(FMargin(20.0f));
    }

    auto AddText = [&](TObjectPtr<UTextBlock>& OutRef, const FString& DefaultText, bool bWrap = false) -> UTextBlock*
    {
        UTextBlock* Block = WidgetTree->ConstructWidget<UTextBlock>();
        if (Block == nullptr)
        {
            return nullptr;
        }
        Block->SetText(FText::FromString(DefaultText));
        Block->SetAutoWrapText(bWrap);
        if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(Block))
        {
            S->SetPadding(FMargin(0.0f, 6.0f));
        }
        OutRef = Block;
        return Block;
    };

    // Title
    UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>();
    if (Title != nullptr)
    {
        Title->SetText(FText::FromString(TEXT("RetroScreen — Setup Required")));
        if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(Title))
        {
            S->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
        }
    }

    AddText(CoreStatusText,      TEXT("[Core] Checking..."));
    AddText(GameDiskStatusText,  TEXT("[Disk] Checking..."));
    AddText(KickstartStatusText, TEXT("[Kickstart] Checking..."));

    // Legal / setup instructions (always shown; becomes most relevant when
    // Kickstart is missing)
    AddText(InstructionsText, FString(KickstartLegalText), /*bWrap=*/true);

    // Dismiss button
    DismissButton = WidgetTree->ConstructWidget<UButton>();
    if (DismissButton != nullptr)
    {
        UTextBlock* BtnLabel = WidgetTree->ConstructWidget<UTextBlock>();
        if (BtnLabel != nullptr)
        {
            BtnLabel->SetText(FText::FromString(TEXT("Dismiss")));
            DismissButton->AddChild(BtnLabel);
        }
        if (UVerticalBoxSlot* S = VBox->AddChildToVerticalBox(DismissButton))
        {
            S->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
        }
        DismissButton->OnClicked.AddDynamic(this, &URetroScreenSetupWidget::HandleDismissClicked);
    }

    bFallbackBuilt = true;
}

// ---------------------------------------------------------------------------

void URetroScreenSetupWidget::RefreshStatusLabels()
{
    if (CoreStatusText != nullptr)
    {
        const FString Msg = CurrentIssues.bCoreMissing
            ? TEXT("[MISSING] Libretro core DLL not found.\n  Set LibretroCorePath in Saved/RetroScreen.ini\n  Expected: Plugins/UnrealLibretro/MyCores/puae_libretro.dll")
            : TEXT("[OK]      Libretro core found.");
        CoreStatusText->SetText(FText::FromString(Msg));
    }

    if (GameDiskStatusText != nullptr)
    {
        const FString Msg = CurrentIssues.bGameDiskMissing
            ? TEXT("[MISSING] Game disk (ADF) not found.\n  Set LibretroRomPath in Saved/RetroScreen.ini\n  Expected: EmulatorData/ADF/disk1.adf")
            : TEXT("[OK]      Game disk found.");
        GameDiskStatusText->SetText(FText::FromString(Msg));
    }

    if (KickstartStatusText != nullptr)
    {
        const FString Msg = CurrentIssues.bKickstartMissing
            ? TEXT("[MISSING] No Kickstart ROM found in EmulatorData/Kickstart/\n  See legal steps below.")
            : TEXT("[OK]      Kickstart ROM found.");
        KickstartStatusText->SetText(FText::FromString(Msg));
    }
}
