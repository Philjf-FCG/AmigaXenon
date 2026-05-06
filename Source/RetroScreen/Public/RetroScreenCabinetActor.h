#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "RetroScreenCabinetActor.generated.h"

class ARetroScreenManager;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class RETROSCREEN_API ARetroScreenCabinetActor : public AActor
{
    GENERATED_BODY()

public:
    ARetroScreenCabinetActor();

    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Cabinet")
    void RefreshCabinetScreenTexture();

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Cabinet")
    void ResolveDefaultCabinetAssets();

protected:
    virtual void BeginPlay() override;

private:
    void ResolveManagerIfNeeded();
    void ResolveMaterialInstancesIfNeeded();

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|Cabinet")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|Cabinet")
    TObjectPtr<UStaticMeshComponent> CabinetMesh;

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|Cabinet")
    TObjectPtr<UStaticMeshComponent> ScreenMesh;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet")
    TObjectPtr<ARetroScreenManager> RetroScreenManager;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet")
    bool bAutoFindManager;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet")
    TObjectPtr<UStaticMesh> CabinetMeshAsset;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet")
    TObjectPtr<UStaticMesh> ScreenMeshAsset;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet")
    TObjectPtr<UMaterialInterface> CabinetBaseMaterial;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet")
    TObjectPtr<UMaterialInterface> ScreenBaseMaterial;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet", meta = (ClampMin = "0"))
    int32 CabinetMaterialSlot;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet", meta = (ClampMin = "0"))
    int32 ScreenMaterialSlot;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet")
    bool bUseSingleMeshScreenSlot;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet", meta = (ClampMin = "0"))
    int32 SingleMeshScreenMaterialSlot;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet")
    FName ScreenTextureParameterName;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CabinetMaterialInstance;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ScreenMaterialInstance;
};
