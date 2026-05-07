#include "ArcadeCabinetActor.h"

#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
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
	, ScreenMode(ECabinetScreenMode::Embedded)
	, bEnableDynamicScreenScaling(true)
	, ScreenHorizontalScale(0.95f)
	, ScreenVerticalScale(0.90f)
	, CabinetMeshAssetPath(TEXT("Fab/Rusty_Japanese_Arcade/rusty_japanese_arcade/StaticMeshes/rusty_japanese_arcade"))
	, ScreenMaterialAssetPath(TEXT(""))
	, CachedViewportWidth(0)
	, CachedViewportHeight(0)
	, bCabinetMeshLoaded(false)
	, ScreenDynamicMaterial(nullptr)
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

	// Update camera configuration for initial viewport
	if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		int32 ViewportWidth = 0;
		int32 ViewportHeight = 0;
		PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);

		if (ViewportWidth > 0 && ViewportHeight > 0)
		{
			CachedViewportWidth = ViewportWidth;
			CachedViewportHeight = ViewportHeight;
			ConfigureForViewport(static_cast<float>(ViewportWidth) / static_cast<float>(ViewportHeight));
		}
	}

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

		// Position cabinet at origin
		CabinetMesh->SetRelativeLocation(FVector::ZeroVector);
		CabinetMesh->SetRelativeRotation(FRotator::ZeroRotator);
		CabinetMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[RetroScreen] Failed to load cabinet mesh from: %s"), *FullAssetPath);
	}
}

void AArcadeCabinetActor::ConfigureForViewport(float AspectRatio)
{
	UpdateScreenTransform(AspectRatio);
	UpdateCameraMode();
}

void AArcadeCabinetActor::UpdateScreenTransform(float AspectRatio)
{
	if (ScreenMode == ECabinetScreenMode::Fullscreen)
	{
		// In fullscreen mode, screen mesh is hidden and camera shows full viewport
		if (ScreenMesh)
		{
			ScreenMesh->SetHiddenInGame(true);
			ScreenMesh->SetVisibility(false, true);
		}
		return;
	}

	// Embedded mode: position screen within cabinet
	if (ScreenMesh)
	{
		// Screen mesh positioned relative to cabinet
		// Coordinates mapped to cabinet screen position
		ScreenMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		ScreenMesh->SetRelativeRotation(FRotator::ZeroRotator);
		ScreenMesh->SetRelativeScale3D(FVector(ScreenHorizontalScale, ScreenVerticalScale, 1.0f));
		ScreenMesh->SetHiddenInGame(false);
		ScreenMesh->SetVisibility(true, true);
	}
}

void AArcadeCabinetActor::UpdateCameraMode()
{
	if (ScreenMode == ECabinetScreenMode::Fullscreen)
	{
		// Fullscreen: camera positioned far back viewing the cabinet straight on
		ViewCamera->SetRelativeLocation(FVector(CameraDistance, CameraHorizontalOffset, CameraHeightOffset));
		ViewCamera->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
		ViewCamera->FieldOfView = CameraFieldOfView;
	}
	else
	{
		// Embedded: camera positioned closer, slight angle
		ViewCamera->SetRelativeLocation(FVector(CameraDistance * 0.7f, CameraHorizontalOffset * 0.5f, CameraHeightOffset * 0.8f));
		ViewCamera->SetRelativeRotation(FRotator(-10.0f, 5.0f, 0.0f));
		ViewCamera->FieldOfView = CameraFieldOfView;
	}
	
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
