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

USTRUCT(BlueprintType)
struct FRetroScreenCrtParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|CRT", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ScanlineIntensity = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|CRT", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Curvature = 0.18f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|CRT", meta = (ClampMin = "0.0", ClampMax = "4.0"))
    float PhosphorBloom = 1.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|CRT", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Vignette = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|CRT", meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float ChromaticAberration = 0.25f;
};

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

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Cabinet|CRT")
    void SetCrtParameters(const FRetroScreenCrtParameters& NewParameters);

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Cabinet|CRT")
    FRetroScreenCrtParameters GetCrtParameters() const { return CrtParameters; }

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Cabinet|CRT")
    void SetCrtEnabled(bool bEnabled);

    UFUNCTION(BlueprintPure, Category = "RetroScreen|Cabinet|CRT")
    bool IsCrtEnabled() const { return bCrtEffectsEnabled; }

protected:
    virtual void BeginPlay() override;

private:
    void ResolveManagerIfNeeded();
    void ResolveMaterialInstancesIfNeeded();
    void ApplyCrtMaterialParameters();

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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|CRT", meta = (AllowPrivateAccess = "true"))
    bool bCrtEffectsEnabled;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|CRT", meta = (AllowPrivateAccess = "true"))
    FRetroScreenCrtParameters CrtParameters;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet|CRT")
    FName CrtEnabledParameterName;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet|CRT")
    FName CrtScanlineIntensityParameterName;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet|CRT")
    FName CrtCurvatureParameterName;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet|CRT")
    FName CrtPhosphorBloomParameterName;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet|CRT")
    FName CrtVignetteParameterName;

    UPROPERTY(EditAnywhere, Category = "RetroScreen|Cabinet|CRT")
    FName CrtChromaticAberrationParameterName;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> CabinetMaterialInstance;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ScreenMaterialInstance;

    bool bCrtParametersDirty;
};
