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
#include "Components/SplineComponent.h"	//Need this for spline component for TeleportPath
#include "Components/SplineMeshComponent.h" //Need this for spline meshes
#include "Saving/FundamentalSaveGame.h"

// Sets default values
AVRCharacter::AVRCharacter()
{
	
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VRRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VRRoot")); //Create inherited component VRRoot which will function as the place where our camera is away from
	VRRoot->SetupAttachment(GetRootComponent());

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera")); //Create inherited component of VRCharacter class
	Camera->SetupAttachment(VRRoot);	//Attach cammera to the root component

	TeleportPath = CreateDefaultSubobject<USplineComponent>(TEXT("TeleportPath")); //Create a teleport position component
	TeleportPath->SetupAttachment(VRRoot);	//attach componenet to the VRRoot

	DestinationMarker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DestinationMarker")); //Create a teleport position component
	DestinationMarker->SetupAttachment(GetRootComponent());	//attach componenet to the VRRoot

	PostProcessComponent = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComponent")); //Create blinkers comoponent
	PostProcessComponent->SetupAttachment(GetRootComponent());	//attach componenet to the VRRoot



}

// Called when the game starts or when spawned
void AVRCharacter::BeginPlay()
{
	Super::BeginPlay();
	DestinationMarker->SetVisibility(false);	//set visibility of DM to be invisible on start

	if (BlinkerMaterialBase != nullptr) //if designer forgot to select base material, so if they left it blank or selected something else
	{
		BlinkerMaterialInstance = UMaterialInstanceDynamic::Create(BlinkerMaterialBase, this);
		PostProcessComponent->AddOrUpdateBlendable(BlinkerMaterialInstance);
	}

	LeftController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (LeftController != nullptr) {
		LeftController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		LeftController->SetHand(EControllerHand::Left);
		LeftController->SetOwner(this);
	}

	RightController = GetWorld()->SpawnActor<AHandController>(HandControllerClass);
	if (RightController != nullptr) {
		RightController->AttachToComponent(VRRoot, FAttachmentTransformRules::KeepRelativeTransform);
		RightController->SetHand(EControllerHand::Right);
		RightController->SetOwner(this);
	}

	LeftController->PairController(RightController);	//Let controllers know about each other
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

// Called to bind functionality to input
void AVRCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	//Movement Inputs
	PlayerInputComponent->BindAxis(TEXT("Forward"), this, &AVRCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Right"), this, &AVRCharacter::MoveRight);
	//Teleport Inputs
	PlayerInputComponent->BindAction(TEXT("Teleport"), IE_Released, this, &AVRCharacter::BeginTeleport);
	//Climbing Inputs
	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Pressed, this, &AVRCharacter::GripLeft);
	PlayerInputComponent->BindAction(TEXT("GripLeft"), IE_Released, this, &AVRCharacter::ReleaseLeft);
	PlayerInputComponent->BindAction(TEXT("GripRight"), IE_Pressed, this, &AVRCharacter::GripRight);
	PlayerInputComponent->BindAction(TEXT("GripRight"), IE_Released, this, &AVRCharacter::ReleaseRight);
	//Drawing VR Inputs
	PlayerInputComponent->BindAction(TEXT("StrokeAButton"), EInputEvent::IE_Pressed, this, &AVRCharacter::RightAButtonPressed);
	PlayerInputComponent->BindAction(TEXT("StrokeAButton"), EInputEvent::IE_Released, this, &AVRCharacter::RightAButtonReleased);
	//Saving Loading Inputs
	PlayerInputComponent->BindAction(TEXT("Save"), EInputEvent::IE_Released, this, &AVRCharacter::Save);
	PlayerInputComponent->BindAction(TEXT("Load"), EInputEvent::IE_Released, this, &AVRCharacter::Load);

}

void AVRCharacter::MoveForward(float throttle)
{
	AddMovementInput(throttle * Camera->GetForwardVector());
}
void AVRCharacter::MoveRight(float throttle)
{
	AddMovementInput(throttle * Camera->GetRightVector());
}

//Detects if LineTrace is successful && ProjectPointToNavigation is successful, Then returns true
bool AVRCharacter::FindTeleportDestination(TArray<FVector>& OutPath, FVector& OutLocation)
{
	FVector Start = RightController->GetActorLocation();	//Sets start of linetracesingle as right controller component location
	FVector Look = RightController->GetActorForwardVector();	//Look Variable is an FVector set to forward vector of right controller

	FPredictProjectilePathParams Params(
		TeleportProjectileRadius,		//Adjustable Variable in header
		Start,							//FVector Start Location from Right Controller
		Look * TeleportProjectileSpeed, //Launch Velocity will be adj. var. from header and Look which is the direction
		TeleportSimulationTime,			//Simulation Time, Idk
		ECollisionChannel::ECC_Visibility,//Sets to visible
		this
	);
	Params.bTraceComplex = true;	//bit of a hack, shouldnt need if created scene from scratch with proper collisions
	FPredictProjectilePathResult Result;

	bool bHit = UGameplayStatics::PredictProjectilePath(this, Params, Result);

	if (!bHit) return false;

	for (FPredictProjectilePathPointData PointData : Result.PathData) {	//Spline Points along the path of FPredictProjectilePathPointData
		OutPath.Add(PointData.Location);
	}

	FNavLocation NavLocation;
	bool bOnNavMesh = UNavigationSystemV1::GetNavigationSystem(GetWorld())->ProjectPointToNavigation(Result.HitResult.Location, NavLocation, TeleportProjectionExtent);

	if (!bOnNavMesh) return false;

	OutLocation = NavLocation.Location;

	return true;
}

void AVRCharacter::UpdateDestinationMarker()
{
	TArray<FVector> Path;	//TAarray of FVectors dictating the path of the spline
	FVector Location;
	bool bHasDestination = FindTeleportDestination(Path, Location);

	if (bHasDestination)//Sets Location of Dest Mark and its visibilty if bOnNavMesh and bHit are true
	{
		DestinationMarker->SetVisibility(true);

		DestinationMarker->SetWorldLocation(Location);	//Destination Marker location is set to worldlocation @ NavLocation

		DrawTeleportPath(Path);	//When bHasDestination is true, Spline Updates
	}
	else
	{
		DestinationMarker->SetVisibility(false);	//if bHit is false, meaning if linetrace doesn't hit anything then DM is invisible
		TArray<FVector> EmptyPath;
		DrawTeleportPath(EmptyPath);	//When bHasDestination is false, just sends empty TArray
	}
}

void AVRCharacter::DrawTeleportPath(const TArray<FVector>& Path) 
{
	UpdateSpline(Path);

	for (USplineMeshComponent* SplineMesh : TeleportPathMeshPool) {
		SplineMesh->SetVisibility(false);
	}

	int32 SegmentNum = Path.Num() - 1;
	for (int32 i = 0; i < SegmentNum; ++i) {
		if (TeleportPathMeshPool.Num() <= i) {
			USplineMeshComponent* SplineMesh = NewObject<USplineMeshComponent>(this); //https://docs.unrealengine.com/4.27/en-US/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/Objects/Creation/
			SplineMesh->SetMobility(EComponentMobility::Movable);	//because we attached a non moveable object to a moveable object (VRRoot) we need to set Mobility for SplineMesh
			SplineMesh->AttachToComponent(TeleportPath, FAttachmentTransformRules::KeepRelativeTransform);	//https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/GameFramework/AActor/AttachToComponent/
			SplineMesh->SetStaticMesh(TeleportArchMesh);
			SplineMesh->SetMaterial(0, TeleportArchMaterial);
			SplineMesh->RegisterComponent(); //this is needed for dynamic objects, any actor is going to have this, whatever that means

			TeleportPathMeshPool.Add(SplineMesh);	//adds the mesh into the TArray
		}

		USplineMeshComponent* SplineMesh = TeleportPathMeshPool[i];
		SplineMesh->SetVisibility(true);

		FVector StartPos, StartTangent, EndPos, EndTangent;
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i, StartPos, StartTangent);
		TeleportPath->GetLocalLocationAndTangentAtSplinePoint(i+1, EndPos, EndTangent);
		SplineMesh->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
	}

}

void AVRCharacter::UpdateSpline(const TArray<FVector>& Path) 
{
	TeleportPath->ClearSplinePoints(false);
	for (int32 i = 0; i < Path.Num(); ++i) {
		FVector LocalPosition = TeleportPath->GetComponentTransform().InverseTransformPosition(Path[i]);	//fixes where the spline is to be not from world location but from local position
		FSplinePoint Point(i, LocalPosition, ESplinePointType::Curve);	//Point Variable: used to dictate what kind of spline is made
		TeleportPath->AddPoint(Point, false);	//Creates a spline path making a new node along the path
	}

	TeleportPath->UpdateSpline();
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
	if (FVector::DotProduct(Camera->GetForwardVector(), MovementDirection) > 0){	//Not sure what a dot product is but in this case it'll allow us to move around forwards backwards and not go blind of blinkers{
		WorldStationaryLocation = Camera->GetComponentLocation() + MovementDirection * 1000;	//Set WorldStationaryLocation to Camera's location plus the movement direction * 1000;
	}
	else{
		WorldStationaryLocation = Camera->GetComponentLocation() - MovementDirection * 1000;	//Set WorldStationaryLocation to Camera's location plus the movement direction * 1000;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC == nullptr)
	{
		return FVector2D(0.5, 0.5);
	}

	FVector2D ScreenStationaryLocaiton;
	PC->ProjectWorldLocationToScreen(WorldStationaryLocation, ScreenStationaryLocaiton);

	int32 SizeX, SizeY;
	PC->GetViewportSize(SizeX, SizeY);
	ScreenStationaryLocaiton.X /= SizeX;
	ScreenStationaryLocaiton.Y /= SizeY;

	return ScreenStationaryLocaiton;
}

void AVRCharacter::BeginTeleport()
{
	StartFade(0, 1); //Fade from white to black

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, this, &AVRCharacter::FinishTeleport, TeleportFadeTime);
}
void AVRCharacter::FinishTeleport()
{
	FVector Destination = DestinationMarker->GetComponentLocation();
	Destination += GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * GetActorUpVector();
	SetActorLocation(Destination);	
	
	StartFade(1, 0); //Reverse Fade In to Fade out. Basically from black to white
}

void AVRCharacter::StartFade(float FromAlpha, float ToAlpha) 
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC != nullptr)
	{
		PC->PlayerCameraManager->StartCameraFade(FromAlpha, ToAlpha, TeleportFadeTime, FLinearColor::Black);
	}
}

void AVRCharacter::Save()
{
	UFundamentalSaveGame* Fundamental = UFundamentalSaveGame::Create();
	Fundamental->SetState("Hello World");
	Fundamental->SerializeFromWorld(GetWorld());
	Fundamental->Save();
}

void AVRCharacter::Load()
{
	UFundamentalSaveGame* Fundamental = UFundamentalSaveGame::Load();
	if (Fundamental) {
		Fundamental->DeserializeToWorld(GetWorld());
		UE_LOG(LogTemp, Warning, TEXT("Painting State %s"), *Fundamental->GetState());
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("Not Found"));
	}
}
