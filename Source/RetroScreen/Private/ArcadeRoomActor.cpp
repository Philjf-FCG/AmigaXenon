#include "ArcadeRoomActor.h"

#include "Components/AudioComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "EngineUtils.h"
#include "Sound/SoundBase.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AArcadeRoomActor::AArcadeRoomActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    auto MakePanel = [this](const FName& Name) -> UStaticMeshComponent*
    {
        UStaticMeshComponent* Comp = CreateDefaultSubobject<UStaticMeshComponent>(Name);
        Comp->SetupAttachment(SceneRoot);
        Comp->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Comp->SetGenerateOverlapEvents(false);
        Comp->SetCastShadow(true);
        return Comp;
    };

    Floor     = MakePanel(TEXT("Floor"));
    BackWall  = MakePanel(TEXT("BackWall"));
    LeftWall  = MakePanel(TEXT("LeftWall"));
    RightWall = MakePanel(TEXT("RightWall"));
    Ceiling   = MakePanel(TEXT("Ceiling"));
    FrontWall = MakePanel(TEXT("FrontWall"));

    AmbientLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("AmbientLight"));
    AmbientLight->SetupAttachment(SceneRoot);
    AmbientLight->SetIntensity(AmbientLightIntensity);
    AmbientLight->SetLightColor(AmbientLightColor);
    AmbientLight->bUseTemperature = false;

    RoomToneAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("RoomToneAudio"));
    RoomToneAudio->SetupAttachment(SceneRoot);
    RoomToneAudio->bAutoActivate = false;
    RoomToneAudio->bIsUISound = false;
    RoomToneAudio->SetVolumeMultiplier(RoomToneVolume);

    ElectricalHumAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("ElectricalHumAudio"));
    ElectricalHumAudio->SetupAttachment(SceneRoot);
    ElectricalHumAudio->bAutoActivate = false;
    ElectricalHumAudio->bIsUISound = false;
    ElectricalHumAudio->SetVolumeMultiplier(ElectricalHumVolume);

    // Load the engine cube at constructor time (safe for CDO)
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Floor->SetStaticMesh(CubeMesh.Object);
        BackWall->SetStaticMesh(CubeMesh.Object);
        LeftWall->SetStaticMesh(CubeMesh.Object);
        RightWall->SetStaticMesh(CubeMesh.Object);
        Ceiling->SetStaticMesh(CubeMesh.Object);
        FrontWall->SetStaticMesh(CubeMesh.Object);
    }
}

void AArcadeRoomActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    BuildRoom();
}

void AArcadeRoomActor::BeginPlay()
{
    Super::BeginPlay();
    BuildRoom();
    StartAmbientAudio();

    // Disable any sky/atmospheric/cloud actors from the default level.
    // This is an indoor arcade room — sky irradiance and reflections bleed onto the emulator screen.
    int32 NumChecked = 0;
    int32 NumDisabled = 0;
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
    {
        AActor* Act = *It;
        if (Act == this) { continue; }
        ++NumChecked;
        const FString CN = Act->GetClass()->GetName();

        // Log all light/sky/cloud class names to diagnose the scene
        if (CN.Contains(TEXT("Light")) || CN.Contains(TEXT("Sky"))
            || CN.Contains(TEXT("Cloud")) || CN.Contains(TEXT("Fog"))
            || CN.Contains(TEXT("Atmosphere")))
        {
            UE_LOG(LogTemp, Display, TEXT("[ArcadeRoom] Scene actor: %s (class=%s)"),
                *Act->GetName(), *CN);
        }

        const bool bSkyRelated = CN.Contains(TEXT("SkyLight"))
            || CN.Contains(TEXT("SkyAtmosphere"))
            || CN.Contains(TEXT("VolumetricCloud"))
            || CN.Contains(TEXT("BP_Sky"))
            || CN.Contains(TEXT("HeightFog"))
            || CN.Contains(TEXT("DirectionalLight"));

        if (bSkyRelated)
        {
            Act->SetActorHiddenInGame(true);
            TArray<ULightComponent*> LCs;
            Act->GetComponents<ULightComponent>(LCs);
            for (ULightComponent* LC : LCs)
            {
                LC->SetVisibility(false);
            }
            ++NumDisabled;
            UE_LOG(LogTemp, Display, TEXT("[ArcadeRoom] Disabled sky actor: %s (class=%s)"),
                *Act->GetName(), *CN);
        }
    }
    UE_LOG(LogTemp, Display, TEXT("[ArcadeRoom] Sky sweep: checked %d actors, disabled %d"),
        NumChecked, NumDisabled);
}

void AArcadeRoomActor::StartAmbientAudio()
{
    if (!bAmbientAudioEnabled)
    {
        return;
    }

    if (RoomToneAudio != nullptr && RoomToneSound != nullptr)
    {
        RoomToneAudio->SetSound(RoomToneSound);
        RoomToneAudio->SetVolumeMultiplier(RoomToneVolume);
        RoomToneAudio->Play();
    }

    if (ElectricalHumAudio != nullptr && ElectricalHumSound != nullptr)
    {
        ElectricalHumAudio->SetSound(ElectricalHumSound);
        ElectricalHumAudio->SetVolumeMultiplier(ElectricalHumVolume);
        ElectricalHumAudio->Play();
    }
}

void AArcadeRoomActor::BuildRoom()
{
    // The engine Cube mesh is 100x100x100 cm.  We scale each panel to the
    // required dimensions.  Positions are relative to the actor root.

    const float W = RoomHalfExtentXY * 2.0f;   // full room width / depth
    const float H = RoomHeight;
    const float T = PanelThickness;

    // --- Floor ---
    // Lies flat (XY plane), centred at room origin, sits just below Z=0
    Floor->SetRelativeLocation(FVector(0.0f, 0.0f, -T * 0.5f));
    Floor->SetRelativeRotation(FRotator::ZeroRotator);
    Floor->SetRelativeScale3D(FVector(W / 100.0f, W / 100.0f, T / 100.0f));

    // --- Ceiling ---
    Ceiling->SetRelativeLocation(FVector(0.0f, 0.0f, H + T * 0.5f));
    Ceiling->SetRelativeRotation(FRotator::ZeroRotator);
    Ceiling->SetRelativeScale3D(FVector(W / 100.0f, W / 100.0f, T / 100.0f));

    // --- Back wall (negative Y face) ---
    BackWall->SetRelativeLocation(FVector(0.0f, -RoomHalfExtentXY - T * 0.5f, H * 0.5f));
    BackWall->SetRelativeRotation(FRotator::ZeroRotator);
    BackWall->SetRelativeScale3D(FVector(W / 100.0f, T / 100.0f, H / 100.0f));

    // --- Left wall (negative X face) ---
    LeftWall->SetRelativeLocation(FVector(-RoomHalfExtentXY - T * 0.5f, 0.0f, H * 0.5f));
    LeftWall->SetRelativeRotation(FRotator::ZeroRotator);
    LeftWall->SetRelativeScale3D(FVector(T / 100.0f, W / 100.0f, H / 100.0f));

    // --- Right wall (positive X face) ---
    RightWall->SetRelativeLocation(FVector(RoomHalfExtentXY + T * 0.5f, 0.0f, H * 0.5f));
    RightWall->SetRelativeRotation(FRotator::ZeroRotator);
    RightWall->SetRelativeScale3D(FVector(T / 100.0f, W / 100.0f, H / 100.0f));

    // --- Front wall (positive Y face) — closes the room so sky is not visible ---
    FrontWall->SetRelativeLocation(FVector(0.0f, RoomHalfExtentXY + T * 0.5f, H * 0.5f));
    FrontWall->SetRelativeRotation(FRotator::ZeroRotator);
    FrontWall->SetRelativeScale3D(FVector(W / 100.0f, T / 100.0f, H / 100.0f));

    // --- Ambient light: ceiling-centre, aimed down ---
    AmbientLight->SetRelativeLocation(FVector(0.0f, 0.0f, H * 0.8f));
    AmbientLight->SetIntensity(AmbientLightIntensity);
    AmbientLight->SetLightColor(AmbientLightColor);
    AmbientLight->AttenuationRadius = FMath::Max(W, H) * 1.5f;

    // --- Apply materials ---
    if (FloorMaterial != nullptr)
    {
        Floor->SetMaterial(0, FloorMaterial);
    }

    if (WallMaterial != nullptr)
    {
        BackWall->SetMaterial(0, WallMaterial);
        LeftWall->SetMaterial(0, WallMaterial);
        RightWall->SetMaterial(0, WallMaterial);
        Ceiling->SetMaterial(0, WallMaterial);
        FrontWall->SetMaterial(0, WallMaterial);
    }
}
