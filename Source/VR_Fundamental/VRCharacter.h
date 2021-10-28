// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "VRCharacter.generated.h"

UCLASS()
class VR_FUNDAMENTAL_API AVRCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AVRCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:									//Function for componenets Instantiation
	bool FindTeleportDestination(TArray<FVector> &OutPath, FVector &OutLocation);
	void UpdateDestinationMarker();
	void UpdateBlinkers();
	void DrawTeleportPath(const TArray<FVector>& Path);
	void UpdateSpline(const TArray<FVector>& Path);	//make const so don't have to make copies of objects
	FVector2D GetBlinkerCentre();

	void MoveForward(float throttle);
	void MoveRight(float throttle);

	void BeginTeleport();
	void FinishTeleport();

	void StartFade(float FromAlpha, float ToAlpha);



private:	//Component Instantiations
	UPROPERTY(VisibleAnywhere)
		class UCameraComponent* Camera;
	UPROPERTY(VisibleAnywhere)
		class AHandController* LeftController;
	UPROPERTY(VisibleAnywhere)
		class AHandController* RightController;
	UPROPERTY(VisibleAnywhere)
		class USceneComponent* VRRoot;
	UPROPERTY(VisibleAnywhere)
		class USplineComponent* TeleportPath;
	UPROPERTY(VisibleAnywhere)
		class UStaticMeshComponent* DestinationMarker;
	UPROPERTY(VisibleAnywhere)
		class UPostProcessComponent* PostProcessComponent;
	UPROPERTY(VisibleAnywhere)
		class UMaterialInstanceDynamic* BlinkerMaterialInstance;
	UPROPERTY(VisibleAnywhere)
		TArray<class USplineMeshComponent*> TeleportPathMeshPool;
	

private:	//Configuration Parameters

	UPROPERTY(EditAnywhere)
		float TeleportProjectileRadius = 10;	//Param for Projectile Path
	UPROPERTY(EditAnywhere)
		float TeleportProjectileSpeed = 800;	//Param for Projectile Path
	UPROPERTY(EditAnywhere)
		float TeleportSimulationTime = 1;	//Param for Projectile Path
	UPROPERTY(EditAnywhere)
		float TeleportFadeTime = 1;	//Fade Timer
	UPROPERTY(EditAnywhere)
		FVector TeleportProjectionExtent = FVector(100,100,100);	//Teleport Extent is 1meter^2
	UPROPERTY(EditAnywhere)
		class UMaterialInterface* BlinkerMaterialBase;	//Blinker Matieral Base is the Parent of the Blinker Material Instance
	UPROPERTY(EditAnywhere)
		class UCurveFloat* RadiusVsVelocity;	//Becomes radius of blinker when moving
	UPROPERTY(EditDefaultsOnly)
		class UStaticMesh* TeleportArchMesh;
	UPROPERTY(EditDefaultsOnly)
		class UMaterialInterface* TeleportArchMaterial;
	UPROPERTY(EditDefaultsOnly)
		TSubclassOf<AHandController> HandControllerClass;
};
