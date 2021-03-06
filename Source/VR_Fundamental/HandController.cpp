// Fill out your copyright notice in the Description page of Project Settings.

#include "HandController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

// Sets default values
AHandController::AHandController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MotionController = CreateDefaultSubobject<UMotionControllerComponent>(TEXT("MotionController"));
	SetRootComponent(MotionController);
}

// Called when the game starts or when spawned
void AHandController::BeginPlay()
{
	Super::BeginPlay();

	OnActorBeginOverlap.AddDynamic(this, &AHandController::ActorBeginOverlap);
	OnActorEndOverlap.AddDynamic(this, &AHandController::ActorEndOverlap);

}

// Called every frame
void AHandController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bIsClimbing) {
		FVector HandControllerDelta = GetActorLocation() - ClimbingStartLocation;

		GetAttachParentActor()->AddActorWorldOffset(-HandControllerDelta);
	}

	if (CurrentStroke) {
		CurrentStroke->Update(GetActorLocation());
	}

}

//AHandController AButton Functions for Stroke Drawing
void AHandController::AButtonPressed()
{
	CurrentStroke = GetWorld()->SpawnActor<AStroke>(StrokeClass);
	CurrentStroke->SetActorLocation(GetActorLocation());
}
void AHandController::AButtonReleased()
{
	CurrentStroke = nullptr; //this just tells us that we are no longer drawing
}

void AHandController::ActorBeginOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bool bNewCanClimb = CanClimb(); //sets value of bCanClimb (true or false) from CanClimb
	if (!bCanClimb && bNewCanClimb) 
	{
		APawn* Pawn = Cast<APawn>(GetAttachParentActor());
		if (Pawn != nullptr) {
			APlayerController* Controller = Cast<APlayerController>(Pawn->GetController());
			if (Controller != nullptr) {
				Controller->PlayHapticEffect(HapticEffect, MotionController->GetTrackingSource());
			}
		}
	}
	bCanClimb = bNewCanClimb;
}

void AHandController::ActorEndOverlap(AActor* OverlappedActor, AActor* OtherActor)
{
	bCanClimb = CanClimb(); //sets value of bCanClimb (true or false) from CanClimb
}


//Check if Tag in your actor class has "Climbable" as a tag
bool AHandController::CanClimb() const
{
	TArray<AActor*> OverlappingActors;
	GetOverlappingActors(OverlappingActors);
	for (AActor* OverlappingActor : OverlappingActors)
	{
		if (OverlappingActor->ActorHasTag(TEXT("Climbable"))) 
		{
			return true;
		}
	}
	return false;

}

void AHandController::Grip() {
	if (!bCanClimb) return;

	if (!bIsClimbing) {
		bIsClimbing = true;
		ClimbingStartLocation = GetActorLocation();

		OtherController->bIsClimbing = false;

		ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
		if (Character != nullptr) {
			Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);

		}
	}
}

void AHandController::Release() {
	if (bIsClimbing) {
		bIsClimbing = false;
		ACharacter* Character = Cast<ACharacter>(GetAttachParentActor());
		if (Character != nullptr) {
			Character->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
		}
	}
}

// Controllers face each other
void AHandController::PairController(AHandController* Controller)
{
	OtherController = Controller;
	OtherController->OtherController = this;
}

