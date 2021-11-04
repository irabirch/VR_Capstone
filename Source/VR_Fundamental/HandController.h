// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MotionControllerComponent.h"
#include "Stroke.h"
#include "HandController.generated.h"

UCLASS()
class VR_FUNDAMENTAL_API AHandController : public AActor
{
	GENERATED_BODY()

	
public:	
	// Sets default values for this actor's properties
	AHandController();

	void SetHand(EControllerHand Hand) { MotionController->SetTrackingSource(Hand); }
	void PairController(AHandController* Controller);

	void AButtonPressed();
	void AButtonReleased();

	void Grip();
	void Release();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	// Callbacks
	UFUNCTION()
	void ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor);
	UFUNCTION()
	void ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor);

	// Config
	UPROPERTY(EditAnywhere)
		TSubclassOf<AStroke> StrokeClass;

	// Helpers
	bool CanClimb() const;

	//Default Sub Object
	UPROPERTY(VisibleAnywhere)
		UMotionControllerComponent* MotionController;

	//Parameters
	UPROPERTY(EditDefaultsOnly)
		class UHapticFeedbackEffect_Base* HapticEffect;

	// State
	bool bCanClimb = false;
	bool bIsClimbing = false;
	FVector ClimbingStartLocation;
	AStroke* CurrentStroke;

	AHandController* OtherController;
};