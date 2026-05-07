#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ArcadeRoomActor.generated.h"

class UPointLightComponent;
class USceneComponent;
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

    UPROPERTY(VisibleAnywhere, Category = "Arcade Room|Lighting")
    TObjectPtr<UPointLightComponent> AmbientLight;

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
};
