#include "RetroScreenCabinetActor.h"

#include "RetroScreenManager.h"

#include "Components/RectLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ARetroScreenCabinetActor::ARetroScreenCabinetActor()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    CabinetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CabinetMesh"));
    CabinetMesh->SetupAttachment(SceneRoot);

    ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
    ScreenMesh->SetupAttachment(CabinetMesh);
    ScreenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ScreenGlowLight = CreateDefaultSubobject<URectLightComponent>(TEXT("ScreenGlowLight"));
    ScreenGlowLight->SetupAttachment(ScreenMesh);
    // Emit in the +Z direction of the screen mesh local space (away from the screen face toward the room).
    // URectLightComponent emits along local -Z, so rotate 180° around X to flip into +Z.
    ScreenGlowLight->SetRelativeLocation(FVector(0.0f, 0.0f, 2.0f));
    ScreenGlowLight->SetRelativeRotation(FRotator(180.0f, 0.0f, 0.0f));
    ScreenGlowLight->SetIntensity(ScreenGlowIntensity);
    ScreenGlowLight->SetLightColor(ScreenGlowColor);
    ScreenGlowLight->AttenuationRadius = ScreenGlowAttenuationRadius;
    ScreenGlowLight->SourceWidth = ScreenGlowSourceWidth;
    ScreenGlowLight->SourceHeight = ScreenGlowSourceHeight;
    ScreenGlowLight->bUseTemperature = false;
    ScreenGlowLight->SetCastShadows(false);

    bAutoFindManager = true;
    CabinetMeshAsset = nullptr;
    ScreenMeshAsset = nullptr;
    CabinetBaseMaterial = nullptr;
    ScreenBaseMaterial = nullptr;
    CabinetMaterialSlot = 0;
    ScreenMaterialSlot = 0;
    bUseSingleMeshScreenSlot = false;
    SingleMeshScreenMaterialSlot = 0;
    ScreenTextureParameterName = TEXT("ScreenTexture");
    bCrtEffectsEnabled = true;
    CrtEnabledParameterName = TEXT("CRT_Enabled");
    CrtScanlineIntensityParameterName = TEXT("CRT_ScanlineIntensity");
    CrtCurvatureParameterName = TEXT("CRT_Curvature");
    CrtPhosphorBloomParameterName = TEXT("CRT_PhosphorBloom");
    CrtVignetteParameterName = TEXT("CRT_Vignette");
    CrtChromaticAberrationParameterName = TEXT("CRT_ChromaticAberration");
    CabinetMaterialInstance = nullptr;
    ScreenMaterialInstance = nullptr;
    bCrtParametersDirty = true;
    CurrentLinkedGlowColor = ScreenGlowColor;
    CurrentLinkedGlowIntensity = ScreenGlowIntensity;
}

void ARetroScreenCabinetActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    ApplyScreenGlowParameters();
}

void ARetroScreenCabinetActor::BeginPlay()
{
    Super::BeginPlay();

    ResolveManagerIfNeeded();
    ResolveDefaultCabinetAssets();
    ResolveMaterialInstancesIfNeeded();
    ApplyCrtMaterialParameters();
    ApplyScreenGlowParameters();
    RefreshCabinetScreenTexture();
}

void ARetroScreenCabinetActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ApplyCrtMaterialParameters();
    RefreshCabinetScreenTexture();

    if (bScreenGlowEnabled && bScreenLinkGlowToContent && ScreenGlowLight != nullptr && RetroScreenManager != nullptr)
    {
        UpdateScreenLinkedGlow(DeltaSeconds);
    }
}

void ARetroScreenCabinetActor::UpdateScreenLinkedGlow(float DeltaSeconds)
{
    const FLinearColor AvgColor = RetroScreenManager->GetAverageScreenColor();

    // Perceptual luminance (Rec. 709 coefficients)
    const float Luminance = AvgColor.R * 0.2126f + AvgColor.G * 0.7152f + AvgColor.B * 0.0722f;
    const float TargetIntensity = ScreenGlowIntensity * FMath::Sqrt(FMath::Clamp(Luminance, 0.0f, 1.0f));

    // Tint the light toward the screen color; fall back to the designer-set color on black frames
    const FLinearColor TargetColor = Luminance > 0.01f ? AvgColor : ScreenGlowColor;

    const float Alpha = FMath::Clamp(ScreenGlowTrackingSpeed * DeltaSeconds, 0.0f, 1.0f);
    CurrentLinkedGlowIntensity = FMath::Lerp(CurrentLinkedGlowIntensity, TargetIntensity, Alpha);
    CurrentLinkedGlowColor = FMath::Lerp(CurrentLinkedGlowColor, TargetColor, Alpha);

    ScreenGlowLight->SetIntensity(CurrentLinkedGlowIntensity);
    ScreenGlowLight->SetLightColor(CurrentLinkedGlowColor);
}

void ARetroScreenCabinetActor::ApplyScreenGlowParameters()
{
    if (ScreenGlowLight == nullptr)
    {
        return;
    }

    ScreenGlowLight->SetVisibility(bScreenGlowEnabled);
    ScreenGlowLight->SetIntensity(bScreenGlowEnabled ? ScreenGlowIntensity : 0.0f);
    ScreenGlowLight->SetLightColor(ScreenGlowColor);
    ScreenGlowLight->AttenuationRadius = ScreenGlowAttenuationRadius;
    ScreenGlowLight->SourceWidth = ScreenGlowSourceWidth;
    ScreenGlowLight->SourceHeight = ScreenGlowSourceHeight;
}

void ARetroScreenCabinetActor::SetScreenGlowEnabled(bool bEnabled)
{
    bScreenGlowEnabled = bEnabled;
    ApplyScreenGlowParameters();
}

void ARetroScreenCabinetActor::SetScreenGlowIntensity(float Intensity)
{
    ScreenGlowIntensity = FMath::Clamp(Intensity, 0.0f, 5000.0f);
    ApplyScreenGlowParameters();
}

void ARetroScreenCabinetActor::SetScreenGlowColor(FLinearColor Color)
{
    ScreenGlowColor = Color;
    ApplyScreenGlowParameters();
}

void ARetroScreenCabinetActor::RefreshCabinetScreenTexture()
{
    ResolveManagerIfNeeded();
    ResolveMaterialInstancesIfNeeded();

    if (RetroScreenManager == nullptr || ScreenMaterialInstance == nullptr)
    {
        return;
    }

    UTexture2D* EmulatorTexture = RetroScreenManager->GetEmulatorTexture();
    if (EmulatorTexture == nullptr)
    {
        return;
    }

    ScreenMaterialInstance->SetTextureParameterValue(ScreenTextureParameterName, EmulatorTexture);
}

void ARetroScreenCabinetActor::SetCrtParameters(const FRetroScreenCrtParameters& NewParameters)
{
    CrtParameters.ScanlineIntensity = FMath::Clamp(NewParameters.ScanlineIntensity, 0.0f, 1.0f);
    CrtParameters.Curvature = FMath::Clamp(NewParameters.Curvature, 0.0f, 1.0f);
    CrtParameters.PhosphorBloom = FMath::Clamp(NewParameters.PhosphorBloom, 0.0f, 4.0f);
    CrtParameters.Vignette = FMath::Clamp(NewParameters.Vignette, 0.0f, 1.0f);
    CrtParameters.ChromaticAberration = FMath::Clamp(NewParameters.ChromaticAberration, 0.0f, 2.0f);
    bCrtParametersDirty = true;
    ApplyCrtMaterialParameters();
}

void ARetroScreenCabinetActor::SetCrtEnabled(bool bEnabled)
{
    if (bCrtEffectsEnabled == bEnabled)
    {
        return;
    }

    bCrtEffectsEnabled = bEnabled;
    bCrtParametersDirty = true;
    ApplyCrtMaterialParameters();
}

void ARetroScreenCabinetActor::ResolveDefaultCabinetAssets()
{
    if (CabinetMesh != nullptr)
    {
        if (CabinetMeshAsset == nullptr)
        {
            CabinetMeshAsset = LoadObject<UStaticMesh>(
                nullptr,
                TEXT("/Game/Fab/Rusty_Japanese_Arcade/rusty_japanese_arcade/StaticMeshes/rusty_japanese_arcade.rusty_japanese_arcade")
            );
        }

        if (CabinetMeshAsset == nullptr)
        {
            CabinetMeshAsset = LoadObject<UStaticMesh>(
                nullptr,
                TEXT("/UnrealLibretro/Mesh/ArcadeCabinet.ArcadeCabinet")
            );
        }

        if (CabinetMeshAsset != nullptr)
        {
            CabinetMesh->SetStaticMesh(CabinetMeshAsset);
        }
    }

    if (ScreenMesh != nullptr)
    {
        if (ScreenMeshAsset == nullptr)
        {
            ScreenMeshAsset = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
        }

        if (ScreenMeshAsset != nullptr)
        {
            ScreenMesh->SetStaticMesh(ScreenMeshAsset);
        }
    }

    if (CabinetBaseMaterial == nullptr)
    {
        CabinetBaseMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/Game/Fab/Rusty_Japanese_Arcade/rusty_japanese_arcade/Materials/Arcade.Arcade")
        );
    }

    if (ScreenBaseMaterial == nullptr)
    {
        ScreenBaseMaterial = LoadObject<UMaterialInterface>(
            nullptr,
            TEXT("/UnrealLibretro/Materials/M_GlowingScreen.M_GlowingScreen")
        );
    }
}

void ARetroScreenCabinetActor::ResolveManagerIfNeeded()
{
    if (RetroScreenManager != nullptr || !bAutoFindManager)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    for (TActorIterator<ARetroScreenManager> It(World); It; ++It)
    {
        RetroScreenManager = *It;
        break;
    }
}

void ARetroScreenCabinetActor::ResolveMaterialInstancesIfNeeded()
{
    if (CabinetMesh != nullptr && CabinetMaterialInstance == nullptr)
    {
        UMaterialInterface* SourceCabinetMaterial = CabinetBaseMaterial;
        if (SourceCabinetMaterial == nullptr)
        {
            SourceCabinetMaterial = CabinetMesh->GetMaterial(CabinetMaterialSlot);
        }

        if (SourceCabinetMaterial != nullptr)
        {
            CabinetMaterialInstance = UMaterialInstanceDynamic::Create(SourceCabinetMaterial, this);
            CabinetMesh->SetMaterial(CabinetMaterialSlot, CabinetMaterialInstance);
        }
    }

    if (ScreenMaterialInstance != nullptr)
    {
        return;
    }

    if (bUseSingleMeshScreenSlot)
    {
        if (CabinetMesh == nullptr)
        {
            return;
        }

        UMaterialInterface* SourceScreenMaterial = ScreenBaseMaterial;
        if (SourceScreenMaterial == nullptr)
        {
            SourceScreenMaterial = CabinetMesh->GetMaterial(SingleMeshScreenMaterialSlot);
        }

        if (SourceScreenMaterial == nullptr)
        {
            return;
        }

        ScreenMaterialInstance = UMaterialInstanceDynamic::Create(SourceScreenMaterial, this);
        CabinetMesh->SetMaterial(SingleMeshScreenMaterialSlot, ScreenMaterialInstance);
        bCrtParametersDirty = true;
        ApplyCrtMaterialParameters();
        return;
    }

    if (ScreenMesh == nullptr)
    {
        return;
    }

    UMaterialInterface* SourceScreenMaterial = ScreenBaseMaterial;
    if (SourceScreenMaterial == nullptr)
    {
        SourceScreenMaterial = ScreenMesh->GetMaterial(ScreenMaterialSlot);
    }

    if (SourceScreenMaterial == nullptr)
    {
        return;
    }

    ScreenMaterialInstance = UMaterialInstanceDynamic::Create(SourceScreenMaterial, this);
    ScreenMesh->SetMaterial(ScreenMaterialSlot, ScreenMaterialInstance);
    bCrtParametersDirty = true;
    ApplyCrtMaterialParameters();
}

void ARetroScreenCabinetActor::ApplyCrtMaterialParameters()
{
    if (!bCrtParametersDirty || ScreenMaterialInstance == nullptr)
    {
        return;
    }

    ScreenMaterialInstance->SetScalarParameterValue(CrtEnabledParameterName, bCrtEffectsEnabled ? 1.0f : 0.0f);
    ScreenMaterialInstance->SetScalarParameterValue(CrtScanlineIntensityParameterName, CrtParameters.ScanlineIntensity);
    ScreenMaterialInstance->SetScalarParameterValue(CrtCurvatureParameterName, CrtParameters.Curvature);
    ScreenMaterialInstance->SetScalarParameterValue(CrtPhosphorBloomParameterName, CrtParameters.PhosphorBloom);
    ScreenMaterialInstance->SetScalarParameterValue(CrtVignetteParameterName, CrtParameters.Vignette);
    ScreenMaterialInstance->SetScalarParameterValue(
        CrtChromaticAberrationParameterName,
        CrtParameters.ChromaticAberration
    );

    bCrtParametersDirty = false;
}
