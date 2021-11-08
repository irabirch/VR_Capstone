// Fill out your copyright notice in the Description page of Project Settings.


#include "FundamentalSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "VR_Fundamental/Stroke.h"

UFundamentalSaveGame* UFundamentalSaveGame::Create()
{
	USaveGame* NewSaveGame = UGameplayStatics::CreateSaveGameObject(StaticClass());
	return Cast<UFundamentalSaveGame>(NewSaveGame);
}

bool UFundamentalSaveGame::Save()
{
	return UGameplayStatics::SaveGameToSlot(this, TEXT("Test"), 0);
}

UFundamentalSaveGame* UFundamentalSaveGame::Load() 
{
	return Cast<UFundamentalSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("TEST"), 0));
}

void UFundamentalSaveGame::SerializeFromWorld(UWorld* World)
{
	Strokes.Empty();
	for (TActorIterator<AStroke> StrokeItr(World); StrokeItr; ++StrokeItr)
	{
		Strokes.Add(StrokeItr->GetClass());
	}
}

void UFundamentalSaveGame::DeserializeToWorld(UWorld* World)
{
	ClearWorld(World);
	for (TSubclassOf<AStroke> StrokeClass : Strokes)
	{
		World->SpawnActor<AStroke>(StrokeClass);
	}
}

void UFundamentalSaveGame::ClearWorld(UWorld* World) 
{
	for (TActorIterator<AStroke> StrokeItr(World); StrokeItr; ++StrokeItr)
	{
		StrokeItr->Destroy();
	}
}