#include "RetroScreenCabinetActor.h"

#include "RetroScreenManager.h"

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
    CabinetMaterialInstance = nullptr;
    ScreenMaterialInstance = nullptr;
}

void ARetroScreenCabinetActor::BeginPlay()
{
    Super::BeginPlay();

    ResolveManagerIfNeeded();
    ResolveDefaultCabinetAssets();
    ResolveMaterialInstancesIfNeeded();
    RefreshCabinetScreenTexture();
}

void ARetroScreenCabinetActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshCabinetScreenTexture();
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
}
