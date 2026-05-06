#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "RetroScreenDisplayPlane.generated.h"

class ARetroScreenManager;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;
class UTexture2D;
class USceneComponent;

UCLASS(BlueprintType, Blueprintable)
class RETROSCREEN_API ARetroScreenDisplayPlane : public AActor
{
    GENERATED_BODY()

public:
    ARetroScreenDisplayPlane();

    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Display")
    void RefreshDisplayTexture();

protected:
    virtual void BeginPlay() override;

private:
    void ResolveManagerIfNeeded();
    void ResolveMeshAssetIfNeeded();
    void ResolveMaterialInstanceIfNeeded();
    void UpdateAspectRatio(UTexture2D* SourceTexture);

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|Display")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|Display")
    TObjectPtr<UStaticMeshComponent> ScreenMesh;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Display")
    TObjectPtr<ARetroScreenManager> RetroScreenManager;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Display")
    bool bAutoFindManager;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Display")
    TObjectPtr<UStaticMesh> PlaneMeshAsset;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Display")
    TObjectPtr<UMaterialInterface> BaseScreenMaterial;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Display")
    FName TextureParameterName;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Display")
    bool bMatchAspectRatio;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ScreenMaterialInstance;

    FVector InitialMeshRelativeScale;
    bool bHasInitialMeshScale;
};
