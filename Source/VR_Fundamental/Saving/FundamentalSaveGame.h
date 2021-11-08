// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "FundamentalSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class VR_FUNDAMENTAL_API UFundamentalSaveGame : public USaveGame
{
	GENERATED_BODY()
		
public:
	static UFundamentalSaveGame* Create();
	bool Save();
	static UFundamentalSaveGame* Load();

	void SetState(FString NewState) { State = NewState; }
	FString GetState() const { return State; }

	void SerializeFromWorld(UWorld* World);
	void DeserializeToWorld(UWorld* World);

private:
	void ClearWorld(UWorld* World);

	//State
	UPROPERTY()
		FString State;

	UPROPERTY()
		TArray<TSubclassOf<class AStroke>> Strokes;
};
