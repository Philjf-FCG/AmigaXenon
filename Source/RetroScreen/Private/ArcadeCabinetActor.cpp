#include "ArcadeCabinetActor.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"
#include "RetroScreenManager.h"

AArcadeCabinetActor::AArcadeCabinetActor()
	: CameraDistance(80.0f)
	, CameraHeightOffset(15.0f)
	, CameraHorizontalOffset(-5.0f)
	, CameraFieldOfView(75.0f)
	, bCameraFacesPositiveX(true)
	, ScreenMode(ECabinetScreenMode::Embedded)
	, bCrtEffectsEnabled(true)
	, bEnableDynamicScreenScaling(true)
	, ScreenHorizontalScale(0.95f)
	, ScreenVerticalScale(0.90f)
	, CabinetMeshAssetPath(TEXT("Fab/Rusty_Japanese_Arcade/rusty_japanese_arcade/StaticMeshes/rusty_japanese_arcade"))
	, ScreenMaterialAssetPath(TEXT("/UnrealLibretro/Materials/M_GlowingScreen.M_GlowingScreen"))
	, CachedViewportWidth(0)
	, CachedViewportHeight(0)
	, bCabinetMeshLoaded(false)
	, ScreenDynamicMaterial(nullptr)
	, RetroScreenManager(nullptr)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root component as static reference point
	USceneComponent* RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	RootSceneComponent->SetRelativeLocation(FVector::ZeroVector);
	RootSceneComponent->SetRelativeRotation(FRotator::ZeroRotator);
	SetRootComponent(RootSceneComponent);

	// Create cabinet mesh component (base structure) at origin
	CabinetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CabinetMesh"));
	CabinetMesh->SetupAttachment(RootComponent);
	CabinetMesh->SetRelativeLocation(FVector::ZeroVector);
	CabinetMesh->SetRelativeRotation(FRotator::ZeroRotator);
	CabinetMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CabinetMesh->SetGenerateOverlapEvents(false);
	CabinetMesh->SetCastShadow(true);
	CabinetMesh->SetVisibility(true);

	// Create camera component positioned to view cabinet
	ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ViewCamera"));
	ViewCamera->SetupAttachment(RootComponent);
	ViewCamera->FieldOfView = CameraFieldOfView;

	// Create screen mesh component (display surface)
	ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
	ScreenMesh->SetupAttachment(CabinetMesh);
	ScreenMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScreenMesh->SetGenerateOverlapEvents(false);
	ScreenMesh->SetCastShadow(false);

	// Initialize screen material parameter name
	ScreenTextureParameterName = TEXT("ScreenTexture");
}

void AArcadeCabinetActor::BeginPlay()
{
	Super::BeginPlay();

	// Load cabinet mesh asset
	LoadCabinetMesh();

	// Update camera configuration for initial viewport.
	// GetViewportSize returns 0 during BeginPlay in PIE before the first frame is rendered,
	// so we always call ConfigureForViewport — falling back to 16:9 if the size isn't ready yet.
	// Tick() will reconfigure when the real dimensions become available.
	if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		int32 ViewportWidth = 0;
		int32 ViewportHeight = 0;
		PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);

		if (ViewportWidth > 0 && ViewportHeight > 0)
		{
			CachedViewportWidth = ViewportWidth;
			CachedViewportHeight = ViewportHeight;
		}
	}

	const float InitialAspect = (CachedViewportWidth > 0 && CachedViewportHeight > 0)
		? static_cast<float>(CachedViewportWidth) / static_cast<float>(CachedViewportHeight)
		: 16.0f / 9.0f;
	ConfigureForViewport(InitialAspect);

	UE_LOG(LogTemp, Display, TEXT("[RetroScreen] Arcade Cabinet initialized at %s"), *GetActorLocation().ToString());
	UE_LOG(LogTemp, Display, TEXT("[RetroScreen] Cabinet mesh visibility: %d, hidden in game: %d"), 
		CabinetMesh ? CabinetMesh->IsVisible() : -1, CabinetMesh ? CabinetMesh->bHiddenInGame : -1);
	UE_LOG(LogTemp, Display, TEXT("[RetroScreen] Cabinet mesh location: %s"), CabinetMesh ? *CabinetMesh->GetComponentLocation().ToString() : TEXT("nullptr"));
	UE_LOG(LogTemp, Display, TEXT("[RetroScreen] Cabinet mesh scale: %s"), CabinetMesh ? *CabinetMesh->GetComponentScale().ToString() : TEXT("nullptr"));
	UE_LOG(LogTemp, Display, TEXT("[RetroScreen] Camera location: %s"), ViewCamera ? *ViewCamera->GetComponentLocation().ToString() : TEXT("nullptr"));
}

void AArcadeCabinetActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Monitor viewport resize events
	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (PlayerController == nullptr)
	{
		return;
	}

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);

	if (ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return;
	}

	// Reconfigure if viewport dimensions changed
	if (ViewportWidth != CachedViewportWidth || ViewportHeight != CachedViewportHeight)
	{
		CachedViewportWidth = ViewportWidth;
		CachedViewportHeight = ViewportHeight;
		ConfigureForViewport(static_cast<float>(ViewportWidth) / static_cast<float>(ViewportHeight));
	}
		// Update screen texture from emulator each frame
		RefreshScreenTexture();
}

void AArcadeCabinetActor::LoadCabinetMesh()
{
	if (bCabinetMeshLoaded || CabinetMesh == nullptr)
	{
		return;
	}

	if (CabinetMeshAssetPath.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[RetroScreen] Cabinet mesh asset path is empty"));
		return;
	}

	// Construct full asset path for content loading (include /Game/ mount point and asset reference)
	FString Filename = FPaths::GetBaseFilename(CabinetMeshAssetPath);
	FString FullAssetPath = FString::Printf(TEXT("/Game/%s.%s"), *CabinetMeshAssetPath, *Filename);

	// Load cabinet mesh
	UStaticMesh* CabinetStaticMesh = LoadObject<UStaticMesh>(nullptr, *FullAssetPath);
	if (CabinetStaticMesh)
	{
		CabinetMesh->SetStaticMesh(CabinetStaticMesh);
		bCabinetMeshLoaded = true;
		UE_LOG(LogTemp, Display, TEXT("[RetroScreen] Cabinet mesh loaded: %s"), *FullAssetPath);

		const TArray<FStaticMaterial>& Mats = CabinetStaticMesh->GetStaticMaterials();
		for (int32 i = 0; i < Mats.Num(); ++i)
		{
			UE_LOG(LogTemp, Display, TEXT("[RetroScreen] Cabinet material slot %d: %s"), i, *Mats[i].MaterialSlotName.ToString());
		}

		// Position cabinet at origin
		CabinetMesh->SetRelativeLocation(FVector::ZeroVector);
		CabinetMesh->SetRelativeRotation(FRotator::ZeroRotator);
		CabinetMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));

		SetupScreenMeshPlane(CabinetStaticMesh->GetBounds());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RetroScreen] Failed to load cabinet mesh from: %s"), *FullAssetPath);
	}
}

void AArcadeCabinetActor::SetupScreenMeshPlane(const FBoxSphereBounds& CabinetBounds)
{
	if (ScreenMesh == nullptr)
	{
		return;
	}

	UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (PlaneMesh == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RetroScreen] Failed to load /Engine/BasicShapes/Plane"));
		return;
	}

	ScreenMesh->SetStaticMesh(PlaneMesh);

	// Position the plane just outside (in front of) the cabinet's +Y face.
	// Placing it inside the bounding box risks z-fighting or the cabinet surface occluding it.
	// Being outside means any transparent/additive pixels in M_GlowingScreen show the
	// opaque cabinet body behind them rather than sky or room background.
	const float FaceY     = CabinetBounds.BoxExtent.Y + 2.0f;  // 2 units in front of front face
	const float ScreenCenterZ = CabinetBounds.Origin.Z + CabinetBounds.BoxExtent.Z * 0.38f;

	// Scale the unit plane (100×100) to cover ~65% of cabinet width and ~45% of cabinet height.
	const float ScaleX = (CabinetBounds.BoxExtent.X * 1.3f)  / 100.0f;
	const float ScaleY = (CabinetBounds.BoxExtent.Z * 0.9f)  / 100.0f;

	ScreenMesh->SetRelativeLocation(FVector(0.0f, FaceY, ScreenCenterZ));
	// Roll -90° rotates the plane's +Z normal to +Y, making it face the camera.
	ScreenMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, -90.0f));
	ScreenMesh->SetRelativeScale3D(FVector(ScaleX, ScaleY, 1.0f));
	ScreenMesh->SetHiddenInGame(false);
	ScreenMesh->SetVisibility(true, true);

	UE_LOG(LogTemp, Display,
		TEXT("[RetroScreen] Screen plane: Y=%.1f Z=%.1f scale=(%.2f,%.2f)"),
		FaceY, ScreenCenterZ, ScaleX, ScaleY);
}

void AArcadeCabinetActor::ConfigureForViewport(float AspectRatio)
{
	UpdateScreenTransform(AspectRatio);
	UpdateCameraMode();
}

void AArcadeCabinetActor::UpdateScreenTransform(float AspectRatio)
{
	if (ScreenMesh == nullptr)
	{
		return;
	}

	// Transform (position/rotation/scale) is owned by SetupScreenMeshPlane().
	// This function only controls visibility based on mode.
	const bool bVisible = (ScreenMode != ECabinetScreenMode::Fullscreen);
	ScreenMesh->SetHiddenInGame(!bVisible);
	ScreenMesh->SetVisibility(bVisible, true);
}

void AArcadeCabinetActor::UpdateCameraMode()
{
	// Compute standoff and height from the actual mesh bounds so any cabinet size works.
	// Default fallback: screen at ~150 units height, standoff ~150 units.
	float StandoffDist = CameraDistance * 2.0f;
	float ScreenCenterZ = 150.0f + CameraHeightOffset;

	if (CabinetMesh && CabinetMesh->GetStaticMesh())
	{
		const FBoxSphereBounds Bounds = CabinetMesh->GetStaticMesh()->GetBounds();
		// The narrow horizontal half-extent is the front-to-back depth; camera stands off from that face.
		const float NarrowExtent = FMath::Min(Bounds.BoxExtent.X, Bounds.BoxExtent.Y);
		StandoffDist = NarrowExtent + CameraDistance * 0.8f;
		// Screen sits in the upper portion of the cabinet (monitor area).
		ScreenCenterZ = Bounds.Origin.Z + Bounds.BoxExtent.Z * 0.4f + CameraHeightOffset;
		UE_LOG(LogTemp, Display, TEXT("[RetroScreen] Cabinet bounds: origin=%s extent=%s -> standoff=%.1f screenZ=%.1f"),
			*Bounds.Origin.ToString(), *Bounds.BoxExtent.ToString(), StandoffDist, ScreenCenterZ);
	}

	// Screen faces the Y axis on this mesh (narrow horizontal dimension = front-to-back depth).
	// bCameraFacesPositiveX true  -> camera at +Y looking -Y (Yaw 270)
	// bCameraFacesPositiveX false -> camera at -Y looking +Y (Yaw  90)
	const float FacingSign = bCameraFacesPositiveX ? 1.0f : -1.0f;
	const float CamYaw     = bCameraFacesPositiveX ? 270.0f : 90.0f;

	if (ScreenMode == ECabinetScreenMode::Fullscreen)
	{
		ViewCamera->SetRelativeLocation(FVector(
			CameraHorizontalOffset * 0.3f, FacingSign * StandoffDist * 1.5f, ScreenCenterZ));
		ViewCamera->SetRelativeRotation(FRotator(-10.0f, CamYaw, 0.0f));
	}
	else
	{
		ViewCamera->SetRelativeLocation(FVector(
			CameraHorizontalOffset * 0.3f, FacingSign * StandoffDist, ScreenCenterZ));
		ViewCamera->SetRelativeRotation(FRotator(-10.0f, CamYaw, 0.0f));
	}
	ViewCamera->FieldOfView = CameraFieldOfView;

	UE_LOG(LogTemp, Display, TEXT("[RetroScreen] UpdateCameraMode - camera relative loc: %s, world loc: %s"),
		*ViewCamera->GetRelativeLocation().ToString(), *ViewCamera->GetComponentLocation().ToString());
}

void AArcadeCabinetActor::GetViewportDimensions(int32& OutWidth, int32& OutHeight) const
{
	OutWidth = CachedViewportWidth;
	OutHeight = CachedViewportHeight;
}

void AArcadeCabinetActor::SetScreenMode(ECabinetScreenMode NewMode)
{
	if (ScreenMode != NewMode)
	{
		ScreenMode = NewMode;

		if (CachedViewportWidth > 0 && CachedViewportHeight > 0)
		{
			ConfigureForViewport(static_cast<float>(CachedViewportWidth) / static_cast<float>(CachedViewportHeight));
		}

		UE_LOG(LogTemp, Display, TEXT("[RetroScreen] Screen mode changed to: %d"), static_cast<int32>(NewMode));
	}
}

void AArcadeCabinetActor::SetCrtParameters(const FRetroScreenCrtParameters& NewParameters)
{
	CrtParameters.ScanlineIntensity = FMath::Clamp(NewParameters.ScanlineIntensity, 0.0f, 1.0f);
	CrtParameters.Curvature = FMath::Clamp(NewParameters.Curvature, 0.0f, 1.0f);
	CrtParameters.PhosphorBloom = FMath::Clamp(NewParameters.PhosphorBloom, 0.0f, 4.0f);
	CrtParameters.Vignette = FMath::Clamp(NewParameters.Vignette, 0.0f, 1.0f);
	CrtParameters.ChromaticAberration = FMath::Clamp(NewParameters.ChromaticAberration, 0.0f, 2.0f);
	ApplyCrtMaterialParameters();
}

void AArcadeCabinetActor::SetCrtEnabled(bool bEnabled)
{
	bCrtEffectsEnabled = bEnabled;
	ApplyCrtMaterialParameters();
}

void AArcadeCabinetActor::ApplyCrtMaterialParameters()
{
	if (ScreenDynamicMaterial == nullptr)
	{
		return;
	}

	ScreenDynamicMaterial->SetScalarParameterValue(TEXT("CRT_Enabled"), bCrtEffectsEnabled ? 1.0f : 0.0f);
	ScreenDynamicMaterial->SetScalarParameterValue(TEXT("CRT_ScanlineIntensity"), CrtParameters.ScanlineIntensity);
	ScreenDynamicMaterial->SetScalarParameterValue(TEXT("CRT_Curvature"), CrtParameters.Curvature);
	ScreenDynamicMaterial->SetScalarParameterValue(TEXT("CRT_PhosphorBloom"), CrtParameters.PhosphorBloom);
	ScreenDynamicMaterial->SetScalarParameterValue(TEXT("CRT_Vignette"), CrtParameters.Vignette);
	ScreenDynamicMaterial->SetScalarParameterValue(TEXT("CRT_ChromaticAberration"), CrtParameters.ChromaticAberration);
}
void AArcadeCabinetActor::RefreshScreenTexture()
{
	if (!RetroScreenManager)
	{
		// Try to find RetroScreenManager in the world
		for (TActorIterator<ARetroScreenManager> ActorItr(GetWorld()); ActorItr; ++ActorItr)
		{
			RetroScreenManager = *ActorItr;
			break;
		}
	}

	if (!RetroScreenManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RetroScreen] RefreshScreenTexture: RetroScreenManager not found"));
		return;
	}

	// Ensure we have a dynamic material instance
	ResolveMaterialInstance();

	if (!ScreenDynamicMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RetroScreen] RefreshScreenTexture: ScreenDynamicMaterial is null"));
		return;
	}

	// Get the current emulator texture from RetroScreenManager
	UTexture2D* EmulatorTexture = RetroScreenManager->GetEmulatorTexture();
	if (EmulatorTexture)
	{
		// Bind the emulator texture to the screen material (updates both ScreenMesh and CabinetMesh slot 1)
		ScreenDynamicMaterial->SetTextureParameterValue(ScreenTextureParameterName, EmulatorTexture);

		// Log once when the texture is first successfully bound so we can confirm the pipeline
		static bool bFirstBind = false;
		if (!bFirstBind)
		{
			bFirstBind = true;
			UE_LOG(LogTemp, Display, TEXT("[RetroScreen] First emulator texture bind: %dx%d, param='%s'"),
				EmulatorTexture->GetSizeX(), EmulatorTexture->GetSizeY(),
				*ScreenTextureParameterName.ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("[RetroScreen] RefreshScreenTexture: emulator texture is null"));
	}
}

void AArcadeCabinetActor::ResolveMaterialInstance()
{
	if (ScreenDynamicMaterial || ScreenMesh == nullptr)
	{
		return;
	}

	if (ScreenMaterial == nullptr && !ScreenMaterialAssetPath.IsEmpty())
	{
		ScreenMaterial = LoadObject<UMaterialInterface>(nullptr, *ScreenMaterialAssetPath);
	}

	UMaterialInterface* SourceMaterial = ScreenMaterial;
	if (SourceMaterial == nullptr)
	{
		SourceMaterial = ScreenMesh->GetMaterial(0);
	}

	// If we don't have a dynamic material yet, create one from the resolved base material
	if (SourceMaterial)
	{
		ScreenDynamicMaterial = UMaterialInstanceDynamic::Create(SourceMaterial, this);
		if (ScreenDynamicMaterial)
		{
			// Apply to the plane mesh overlay (if present)
			ScreenMesh->SetMaterial(0, ScreenDynamicMaterial);

			// Apply directly to cabinet mesh slot 1 ("Material" = built-in screen surface on the 3D model).
			// This is more reliable than the plane overlay since UVs and position are baked into the mesh.
			if (CabinetMesh && CabinetMesh->GetNumMaterials() > 1)
			{
				CabinetMesh->SetMaterial(1, ScreenDynamicMaterial);
				UE_LOG(LogTemp, Display, TEXT("[RetroScreen] Applied screen material to cabinet mesh slot 1"));
			}

			UE_LOG(LogTemp, Display, TEXT("[RetroScreen] Created dynamic material instance for screen"));
			ApplyCrtMaterialParameters();
		}
	}
}
