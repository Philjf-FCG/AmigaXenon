#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "RetroScreenCabinetActor.generated.h"

class ARetroScreenManager;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class URectLightComponent;
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

    virtual void OnConstruction(const FTransform& Transform) override;
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

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Cabinet|Lighting")
    void SetScreenGlowEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Cabinet|Lighting")
    void SetScreenGlowIntensity(float Intensity);

    UFUNCTION(BlueprintCallable, Category = "RetroScreen|Cabinet|Lighting")
    void SetScreenGlowColor(FLinearColor Color);

protected:
    virtual void BeginPlay() override;

private:
    void ResolveManagerIfNeeded();
    void ResolveMaterialInstancesIfNeeded();
    void ApplyCrtMaterialParameters();
    void ApplyScreenGlowParameters();
    void UpdateScreenLinkedGlow(float DeltaSeconds);

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|Cabinet")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|Cabinet")
    TObjectPtr<UStaticMeshComponent> CabinetMesh;

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|Cabinet")
    TObjectPtr<UStaticMeshComponent> ScreenMesh;

    UPROPERTY(VisibleAnywhere, Category = "RetroScreen|Cabinet|Lighting")
    TObjectPtr<URectLightComponent> ScreenGlowLight;

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

    /** Enable/disable the CRT screen glow rect light that illuminates nearby geometry. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|Lighting", meta = (AllowPrivateAccess = "true"))
    bool bScreenGlowEnabled = true;

    /** Luminous intensity of the screen glow rect light in Lumen. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|Lighting", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "5000.0"))
    float ScreenGlowIntensity = 400.0f;

    /** Tint of the screen glow light; default is a slightly cool CRT blue-white. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|Lighting", meta = (AllowPrivateAccess = "true"))
    FLinearColor ScreenGlowColor = FLinearColor(0.8f, 0.95f, 1.0f);

    /** Radius over which the screen glow fades to zero (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|Lighting", meta = (AllowPrivateAccess = "true", ClampMin = "50.0", ClampMax = "1000.0"))
    float ScreenGlowAttenuationRadius = 250.0f;

    /**
     * When true, drives the screen glow light color and intensity from the average
     * screen luminance each frame, giving dynamic ambient response to on-screen content.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|Lighting", meta = (AllowPrivateAccess = "true"))
    bool bScreenLinkGlowToContent = true;

    /** How quickly the screen-linked light tracks luminance changes (lerp speed per second). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|Lighting", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", ClampMax = "30.0"))
    float ScreenGlowTrackingSpeed = 8.0f;

    /** Width of the emissive rect light source matching the screen face (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|Lighting", meta = (AllowPrivateAccess = "true", ClampMin = "5.0", ClampMax = "200.0"))
    float ScreenGlowSourceWidth = 30.0f;

    /** Height of the emissive rect light source matching the screen face (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RetroScreen|Cabinet|Lighting", meta = (AllowPrivateAccess = "true", ClampMin = "5.0", ClampMax = "200.0"))
    float ScreenGlowSourceHeight = 22.0f;

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
    FLinearColor CurrentLinkedGlowColor;
    float CurrentLinkedGlowIntensity;
};
