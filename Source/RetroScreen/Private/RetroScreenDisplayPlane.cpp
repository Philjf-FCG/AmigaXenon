#include "RetroScreenDisplayPlane.h"

#include "RetroScreenManager.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ARetroScreenDisplayPlane::ARetroScreenDisplayPlane()
{
    PrimaryActorTick.bCanEverTick = true;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
    ScreenMesh->SetupAttachment(SceneRoot);
    ScreenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    bAutoFindManager = true;
    TextureParameterName = TEXT("ScreenTexture");
    bMatchAspectRatio = true;
    InitialMeshRelativeScale = FVector::OneVector;
    bHasInitialMeshScale = false;
}

void ARetroScreenDisplayPlane::BeginPlay()
{
    Super::BeginPlay();

    ResolveManagerIfNeeded();
    ResolveMeshAssetIfNeeded();
    ResolveMaterialInstanceIfNeeded();

    if (ScreenMesh != nullptr)
    {
        InitialMeshRelativeScale = ScreenMesh->GetRelativeScale3D();
        bHasInitialMeshScale = true;
    }

    RefreshDisplayTexture();
}

void ARetroScreenDisplayPlane::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    RefreshDisplayTexture();
}

void ARetroScreenDisplayPlane::RefreshDisplayTexture()
{
    if (ScreenMesh == nullptr)
    {
        return;
    }

    ResolveManagerIfNeeded();
    ResolveMaterialInstanceIfNeeded();

    if (RetroScreenManager == nullptr || ScreenMaterialInstance == nullptr)
    {
        return;
    }

    UTexture2D* EmulatorTexture = RetroScreenManager->GetEmulatorTexture();
    if (EmulatorTexture == nullptr)
    {
        return;
    }

    ScreenMaterialInstance->SetTextureParameterValue(TextureParameterName, EmulatorTexture);

    if (bMatchAspectRatio)
    {
        UpdateAspectRatio(EmulatorTexture);
    }
}

void ARetroScreenDisplayPlane::ResolveManagerIfNeeded()
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

void ARetroScreenDisplayPlane::ResolveMeshAssetIfNeeded()
{
    if (ScreenMesh == nullptr)
    {
        return;
    }

    if (PlaneMeshAsset == nullptr)
    {
        PlaneMeshAsset = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
    }

    if (PlaneMeshAsset != nullptr && ScreenMesh->GetStaticMesh() == nullptr)
    {
        ScreenMesh->SetStaticMesh(PlaneMeshAsset);
    }
}

void ARetroScreenDisplayPlane::ResolveMaterialInstanceIfNeeded()
{
    if (ScreenMesh == nullptr || ScreenMaterialInstance != nullptr)
    {
        return;
    }

    UMaterialInterface* SourceMaterial = BaseScreenMaterial;
    if (SourceMaterial == nullptr)
    {
        SourceMaterial = ScreenMesh->GetMaterial(0);
    }

    if (SourceMaterial == nullptr)
    {
        return;
    }

    ScreenMaterialInstance = UMaterialInstanceDynamic::Create(SourceMaterial, this);
    if (ScreenMaterialInstance != nullptr)
    {
        ScreenMesh->SetMaterial(0, ScreenMaterialInstance);
    }
}

void ARetroScreenDisplayPlane::UpdateAspectRatio(UTexture2D* SourceTexture)
{
    if (SourceTexture == nullptr || ScreenMesh == nullptr)
    {
        return;
    }

    const int32 Width = SourceTexture->GetSizeX();
    const int32 Height = SourceTexture->GetSizeY();
    if (Width <= 0 || Height <= 0)
    {
        return;
    }

    if (!bHasInitialMeshScale)
    {
        InitialMeshRelativeScale = ScreenMesh->GetRelativeScale3D();
        bHasInitialMeshScale = true;
    }

    const float AspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
    const float SafeAspect = FMath::Max(AspectRatio, KINDA_SMALL_NUMBER);
    ScreenMesh->SetRelativeScale3D(
        FVector(
            InitialMeshRelativeScale.X,
            InitialMeshRelativeScale.Y / SafeAspect,
            InitialMeshRelativeScale.Z
        )
    );
}
