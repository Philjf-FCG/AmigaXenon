#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ArcadeRoomActor.generated.h"

class UAudioComponent;
class UMaterialInterface;
class UPointLightComponent;
class USceneComponent;
class USoundBase;
class UStaticMeshComponent;

/**
 * AArcadeRoomActor - Self-contained arcade room environment.
 *
 * Assembles a room shell (floor, back wall, two side walls, ceiling) from
 * engine cube primitives scaled to RoomExtent, with configurable materials.
 * A single point light provides ambient fill.  Drop this actor into the level
 * and position it so the hero cabinet sits roughly centred inside it.
 */
UCLASS(BlueprintType, Blueprintable)
class RETROSCREEN_API AArcadeRoomActor : public AActor
{
    GENERATED_BODY()

public:
    AArcadeRoomActor();

    virtual void OnConstruction(const FTransform& Transform) override;

protected:
    virtual void BeginPlay() override;

private:
    void BuildRoom();
    void StartAmbientAudio();

    UPROPERTY(VisibleAnywhere, Category = "Arcade Room")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Arcade Room")
    TObjectPtr<UStaticMeshComponent> Floor;

    UPROPERTY(VisibleAnywhere, Category = "Arcade Room")
    TObjectPtr<UStaticMeshComponent> BackWall;

    UPROPERTY(VisibleAnywhere, Category = "Arcade Room")
    TObjectPtr<UStaticMeshComponent> LeftWall;

    UPROPERTY(VisibleAnywhere, Category = "Arcade Room")
    TObjectPtr<UStaticMeshComponent> RightWall;

    UPROPERTY(VisibleAnywhere, Category = "Arcade Room")
    TObjectPtr<UStaticMeshComponent> Ceiling;

    UPROPERTY(VisibleAnywhere, Category = "Arcade Room")
    TObjectPtr<UStaticMeshComponent> FrontWall;

    UPROPERTY(VisibleAnywhere, Category = "Arcade Room|Lighting")
    TObjectPtr<UPointLightComponent> AmbientLight;

    UPROPERTY(VisibleAnywhere, Category = "Arcade Room|Audio")
    TObjectPtr<UAudioComponent> RoomToneAudio;

    UPROPERTY(VisibleAnywhere, Category = "Arcade Room|Audio")
    TObjectPtr<UAudioComponent> ElectricalHumAudio;

public:
    /** Half-extent of the room in each horizontal axis (cm). Full room = 2x this. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room", meta = (ClampMin = "100.0"))
    float RoomHalfExtentXY = 400.0f;

    /** Full room height (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room", meta = (ClampMin = "100.0"))
    float RoomHeight = 500.0f;

    /** Thickness of each wall/floor/ceiling panel (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room", meta = (ClampMin = "1.0"))
    float PanelThickness = 10.0f;

    /** Material applied to the floor panel. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room|Materials")
    TObjectPtr<UMaterialInterface> FloorMaterial;

    /** Material applied to walls and ceiling. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room|Materials")
    TObjectPtr<UMaterialInterface> WallMaterial;

    /** Ambient point light intensity (lux). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room|Lighting", meta = (ClampMin = "0.0"))
    float AmbientLightIntensity = 1500.0f;

    /** Ambient point light colour. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room|Lighting")
    FLinearColor AmbientLightColor = FLinearColor(1.0f, 0.9f, 0.8f);

    /** Enable background room-tone and electrical-hum ambient layers. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room|Audio")
    bool bAmbientAudioEnabled = true;

    /** Sound asset for the continuous room-tone loop (low background noise, footsteps, distant machines). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room|Audio")
    TObjectPtr<USoundBase> RoomToneSound;

    /** Sound asset for the electrical / CRT hum loop distinct from emulator audio. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room|Audio")
    TObjectPtr<USoundBase> ElectricalHumSound;

    /** Volume multiplier for the room-tone layer. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float RoomToneVolume = 0.35f;

    /** Volume multiplier for the electrical-hum layer. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Room|Audio", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float ElectricalHumVolume = 0.2f;
};
