#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RetroScreenCabinetActor.h"
#include "ArcadeCabinetActor.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTexture2D;

UENUM(BlueprintType)
enum class ECabinetScreenMode : uint8
{
	Embedded UMETA(DisplayName = "Embedded (Screen in Cabinet)"),
	Fullscreen UMETA(DisplayName = "Fullscreen (Screen Fills View)")
};

/**
 * ArcadeCabinetActor - Displays RetroScreen emulated games on a 3D arcade cabinet model.
 * 
 * Features:
 * - Uses Rusty Japanese Arcade cabinet model with embedded screen display
 * - Dynamic screen texture binding from RetroScreen emulator
 * - Automatic viewport scaling and camera positioning
 * - Integrates with RetroScreenManager for pause menu overlay
 * - Optional fullscreen mode for immersive gameplay
 * - Configurable camera position and field of view
 */
UCLASS(Blueprintable)
class RETROSCREEN_API AArcadeCabinetActor : public AActor
{
	GENERATED_BODY()

public:
	AArcadeCabinetActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/**
	 * Configure cabinet and camera for current viewport aspect ratio
	 */
	void ConfigureForViewport(float AspectRatio);

	/**
	 * Get the screen mesh component (for binding emulator texture)
	 */
	UFUNCTION(BlueprintCallable, Category = "Arcade Cabinet")
	UStaticMeshComponent* GetScreenMeshComponent() const { return ScreenMesh; }

	/**
	 * Get the current viewport dimensions
	 */
	UFUNCTION(BlueprintCallable, Category = "Arcade Cabinet")
	void GetViewportDimensions(int32& OutWidth, int32& OutHeight) const;

	/**
	 * Set screen display mode (embedded or fullscreen)
	 */
	UFUNCTION(BlueprintCallable, Category = "Arcade Cabinet")
	void SetScreenMode(ECabinetScreenMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Arcade Cabinet|CRT")
	void SetCrtParameters(const FRetroScreenCrtParameters& NewParameters);

	UFUNCTION(BlueprintPure, Category = "Arcade Cabinet|CRT")
	FRetroScreenCrtParameters GetCrtParameters() const { return CrtParameters; }

	UFUNCTION(BlueprintCallable, Category = "Arcade Cabinet|CRT")
	void SetCrtEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Arcade Cabinet|CRT")
	bool IsCrtEnabled() const { return bCrtEffectsEnabled; }

	/** Camera viewing the cabinet */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet")
	UCameraComponent* ViewCamera;

	/** Cabinet base model (meshes, structure) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet")
	UStaticMeshComponent* CabinetMesh;

	/** Screen display surface (embedded in cabinet) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet")
	UStaticMeshComponent* ScreenMesh;

	/** Material applied to screen mesh for texture binding */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Display")
	UMaterialInterface* ScreenMaterial;

	/** Parameter name for screen texture in material */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Display")
	FName ScreenTextureParameterName;

	/** Camera distance from cabinet screen (world units) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Camera", meta = (ClampMin = "10.0"))
	float CameraDistance;

	/** Camera height offset (world units) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Camera")
	float CameraHeightOffset;

	/** Camera horizontal offset (world units) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Camera")
	float CameraHorizontalOffset;

	/** Camera field of view (degrees) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Camera", meta = (ClampMin = "5.0", ClampMax = "170.0"))
	float CameraFieldOfView;

	/** Whether the cabinet screen faces the +X direction; disable if camera shows the back face. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Camera")
	bool bCameraFacesPositiveX;

	/** Current screen display mode */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Display")
	ECabinetScreenMode ScreenMode;

	/** Enable CRT post-process effects on screen material */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Cabinet|CRT")
	bool bCrtEffectsEnabled;

	/** CRT visual parameters applied to the screen dynamic material */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arcade Cabinet|CRT")
	FRetroScreenCrtParameters CrtParameters;

	/** Enable dynamic screen sizing based on viewport aspect ratio */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Display")
	bool bEnableDynamicScreenScaling;

	/** Horizontal scale factor for embedded screen (1.0 = normal) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Display", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float ScreenHorizontalScale;

	/** Vertical scale factor for embedded screen (1.0 = normal) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Display", meta = (ClampMin = "0.5", ClampMax = "2.0"))
	float ScreenVerticalScale;

	/** Relative path to cabinet model within Content folder */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Assets")
	FString CabinetMeshAssetPath;

	/** Relative path to screen material within Content folder */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Arcade Cabinet|Assets")
	FString ScreenMaterialAssetPath;

private:
	/** Cached viewport dimensions for resize detection */
	int32 CachedViewportWidth;

	/** Cached viewport dimensions for resize detection */
	int32 CachedViewportHeight;

	/** Whether cabinet mesh was successfully loaded */
	bool bCabinetMeshLoaded;

	/** Material instance for screen texture binding */
	UPROPERTY(Transient)
	UMaterialInstanceDynamic* ScreenDynamicMaterial;

	/** Load cabinet static mesh asset */
	void LoadCabinetMesh();

	/** Update screen position and scale based on mode */
	void UpdateScreenTransform(float AspectRatio);

	/** Switch between embedded and fullscreen camera setup */
	void UpdateCameraMode();

	/** Refresh screen texture from RetroScreenManager emulator output */
	void RefreshScreenTexture();

	/** Resolve or create dynamic material instance for screen */
	void ResolveMaterialInstance();

	/** Assign a plane mesh to ScreenMesh and position it at the cabinet screen opening */
	void SetupScreenMeshPlane(const FBoxSphereBounds& CabinetBounds);

	/** Apply current CRT parameters to the dynamic material instance */
	void ApplyCrtMaterialParameters();
		
		/** Reference to RetroScreen manager for emulator texture */
		UPROPERTY(Transient)
		class ARetroScreenManager* RetroScreenManager;
};

