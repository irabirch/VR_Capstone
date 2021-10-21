// Fill out your copyright notice in the Description page of Project Settings.


#include "VRCharacter.h"
#include "Camera/CameraComponent.h"	//Need this to attach camera to the character
#include "Components/InputComponent.h" //Need this to set up movement in character
#include "Components/StaticMeshComponent.h"	//Need this to set up Teleportation destination. Static mesh is the ring we will teleport to
#include "TimerManager.h"	//Need this to add a timing function to the teleporting function
#include "Components/CapsuleComponent.h"	//Need this to add Height to our Destaion Marker after teleporting so we are not halfway through the ground
#include "NavigationSystem.h"	//Need this teleport to only NavMeshes
#include "Components/PostProcessComponent.h"	//Need this to add blinkers as blinkers are a post processing component
#include "Materials/MaterialInstanceDynamic.h"	//Need this to add blinkers to character instead of world
#include "MotionControllerComponent.h" //Need this to add oculus controllers to game
#include "Kismet/GameplayStatics.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	//"You can view your componenets by looking at your VRCharacter's Details pane in Unreal Engine
	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot")); //Create inherited component VRRoot which will function as the place where our camera is away from
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera")); //Create inherited component of VRCharacter class
	Camera->SetupAttachment(VRRoot);	//Attach cammera to the root component

	LeftController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("LeftController")); //Create Left Controller Component
	LeftController->SetupAttachment(VRRoot);
	LeftController->SetTrackingSource(EControllerHand::Left);	//necessary to set up which hand it is
	LeftController->bDisplayDeviceModel = true;	//sets controller to be visible

	RightController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("RightController")); //Create Right Controller Component
	RightController->SetupAttachment(VRRoot);
	RightController->SetTrackingSource(EControllerHand::Right);	//necessary to set up which hand it is
	RightController->bDisplayDeviceModel = true;	//sets contoller to be visible

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker")); //Create a teleport position component
	DestinationMarker->SetupAttachment(GetRootComponent());	//attach componenet to the VRRoot

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent")); //Create a teleport position component
	PostProcessComponent->SetupAttachment(GetRootComponent());	//attach componenet to the VRRoot

}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	DestinationMarker->SetVisibility(false);	//set visibility of DM to be invisible on start

	/*static UMaterialInstanceDynamic* Create
	(
		class UMaterialInterface* ParentMaterial,
		class UObject* InOuter
	)*/
	if (BlinkerMaterialBase != nullptr) //if designer forgot to select base material, so if they left it blank or selected something else
	{
		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);
	}
}

// Called every frame
void AVRCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewCameraOffset = Camera->GetComponentLocation() - GetActorLocation(); //offset = camera's location we created minus the actors location
	NewCameraOffset.Z = 0;	//like in blueprints, we only want to edit the x and y axis of the VRRoot component location. So set NewCameraOffset Z value to 0, which is default
	AddActorWorldOffset(NewCameraOffset);
	VRRoot->AddWorldOffset(-NewCameraOffset);	//put offset as negative of added actor world offset (NewCameraOffset) setting as VRRoot Location in world

	UpdateDestinationMarker();	//Call Update Dest Mark every tick
	UpdateBlinkers();	//Call update blinkers every tick
}

//Detects if LineTrace is successful && ProjectPointToNavigation is successful, Then returns true
bool AVRCharacter::FindTeleportDestination(FVector& OutLocation) 
{
	FVector Start = RightController->GetComponentLocation();	//Sets start of linetracesingle as right controller component location
	FVector Look = RightController->GetForwardVector();	//Look Variable is an FVector set to forward vector of right controller
	//Look = Look.RotateAngleAxis(30, RightController->GetRightVector());	//Set Look to forward but rotated on axis by 30 degrees, by right controller's right vector
	//FVector End = Start + Look * MaxTeleportDistance;	//Where the teleport ring appears after angling and pointing with right controller

	/*FPredictProjectilePathParams
	(
		float InProjectileRadius,
		FVector InStartLocation,
		FVector InLaunchVelocity,
		float InMaxSimTime,
		ECollisionChannel InTraceChannel,
		AActor * ActorToIgnore
	)*/
	FPredictProjectilePathParams Params(
		TeleportProjectileRadius,		//Adjustable Variable in header
		Start,							//FVector Start Location from Right Controller
		Look * TeleportProjectileSpeed, //Launch Velocity will be adj. var. from header and Look which is the direction
		TeleportSimulationTime,			//Simulation Time, Idk
		ECollisionChannel::ECC_Visibility,//Sets to visible
		this
	);
	Params.DrawDebugType = EDrawDebugTrace::ForOneFrame;	//debug line for Params, will show the Predicted Projectile Path
	Params.bTraceComplex = true;	//bit of a hack, shouldnt need if created scene from scratch with proper collisions
	FPredictProjectilePathResult Result;
	/*static bool PredictProjectilePath
	(
		const UObject * WorldContextObject,
		const FPredictProjectilePathParams & PredictParams,
		FPredictProjectilePathResult & PredictResult
	)*/
	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);

	if (!bHit) return false;

	/*bool ProjectPointToNavigation
	(
		const FVector & Point,
		FNavLocation & OutLocation,
		const FVector & Extent,
		const FNavAgentProperties * AgentProperties,
		FSharedConstNavQueryFilter QueryFilter
	) */
	FNavLocation NavLocation;
	bool bOnNavMesh = UNavigationSystemV1::GetNavigationSystem(GetWorld())->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, TeleportProjectionExtent);

	OutLocation = NavLocation.Location;

	return true;
}



/* This function bool used to get HitResult or Destination Markers location using LineTraceSingleChannel
bool LineTraceSingleByObjectType
(
	struct FHitResult& OutHit,
	const FVector& Start,
	const FVector& End,
	const FCollisionObjectQueryParams& ObjectQueryParams,
	const FCollisionQueryParams& Params
) const*/
void AVRCharacter::UpdateDestinationMarker()
{
	FVector Location;
	bool bHasDestination = FindTeleportDestination(Location);

	if (bHasDestination)//Sets Location of Dest Mark and its visibilty if bOnNavMesh and bHit are true
	{
		DestinationMarker->SetVisibility(true);

		DestinationMarker->SetWorldLocation(Location);	//Destination Marker location is set to worldlocation @ NavLocation
	}
	else
	{
		DestinationMarker->SetVisibility(false);	//if bHit is false, meaning if linetrace doesn't hit anything then DM is invisible
	}
}

void AVRCharacter::UpdateBlinkers()
{
	if (RadiusVsVelocity == nullptr) return;

	float Speed = GetVelocity().Size();	//Gets the speed your moving as a value with .Size()
	float Radius = RadiusVsVelocity->GetFloatValue(Speed);	//Changes Radius by assigning Speed size to RadiusVsVelocity into variable, Radius.

	BlinkerMaterialInstance->SetScalarParameterValue(TEXT("Radius"), Radius);	//Just like in Blueprints where you add a paramenter block in material editor

	FVector2D Centre = GetBlinkerCentre();
	BlinkerMaterialInstance->SetVectorParameterValue(TEXT("Centre"), FLinearColor(Centre.X, Centre.Y,0));
}

FVector2D AVRCharacter::GetBlinkerCentre()
{
	FVector MovementDirection = GetVelocity().GetSafeNormal();	//Set MovementDirection to SafeNormal of Velocity
	if (MovementDirection.IsNearlyZero()) //Can't set normal to zero as it can only get close to zero, idk something about normals
	{
		return FVector2D(0.5, 0.5);
	}

	FVector WorldStationaryLocation;
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0)	//Not sure what a dot product is but in this case it'll allow us to move around forwards backwards and not go blind of blinkers
	{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;	//Set WorldStationaryLocation to Camera's location plus the movement direction * 1000;
	}
	else
	{
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000;	//Set WorldStationaryLocation to Camera's location plus the movement direction * 1000;

	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr)
	{
		return FVector2D(0.5, 0.5);
	}

	/*bool ProjectWorldLocationToScreen
	(
		FVector WorldLocation,
		FVector2D & ScreenLocation,
		bool bPlayerViewportRelative
	) const */
	FVector2D ScreenStationaryLocaiton;
	PC->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocaiton);

	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);
	ScreenStationaryLocaiton.X /= SizeX;
	ScreenStationaryLocaiton.Y /= SizeY;

	return ScreenStationaryLocaiton;
}



// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	PlayerInputComponent->BindAction(TEXT("Teleport"),IE_Released, this, &AVRCharacter::BeginTeleport);
}
void AVRCharacter::MoveForward(float throttle) 
{
	AddMovementInput(throttle * Camera->GetForwardVector());
}
void AVRCharacter::MoveRight(float throttle)
{
	AddMovementInput(throttle * Camera->GetRightVector());
}

/*template<class UserClass>
void SetTimer
(
	FTimerHandle & InOutHandle,
	UserClass * InObj,
	typename FTimerDelegate::TUObjectMethodDelegate< UserClass >::FMethodPtr InTimerMethod,
	float InRate,
	bool InbLoop,
	float InFirstDelay
)*/
void AVRCharacter::BeginTeleport()
{
	//Fade from white to black
	StartFade(0, 1);

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FinishTeleport, TeleportFadeTime);
}
void AVRCharacter::FinishTeleport()
{	//Sets Location of Actor to where Destination Marker is and adds capsule half height so we are not inside the mesh
	SetActorLocation(DestinationMarker->GetComponentLocation() + GetCapsuleComponent()->GetScaledCapsuleHalfHeight());	
	//Reverse Fade In to Fade out. Basically from black to white
	StartFade(1, 0);
}


/*virtual void StartCameraFade
(
	float FromAlpha,
	float ToAlpha,
	float Duration,
	FLinearColor Color,
	bool bShouldFadeAudio,
	bool bHoldWhenFinished
)*/
void AVRCharacter::StartFade(float FromAlpha, float ToAlpha) 
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC != nullptr)
	{
		PC->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeTime, FLinearColor::Black);
	}
}
