// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SlotData.h"
#include "SavePreset.h"
#include "TimerManager.h"


/**
 * A macro for tidying up accessing of private members, through the above code
 *
 * @param InClass		The class being accessed (not a string, just the class, i.e. FStackTracker)
 * @param InObj			Pointer to an instance of the specified class
 * @param MemberName	Name of the member being accessed (again, not a string)
 * @return				The value of the member
 */
#define GET_PRIVATE(InClass, InObj, MemberName) (*InObj).*GetPrivate(InClass##MemberName##Accessor())


void FLevelRecord::Clean()
{
	LevelScript = {};
	Actors.Empty();
	AIControllers.Empty();
}


FName FPersistentLevelRecord::PersistentName{ "Persistent" };


/////////////////////////////////////////////////////
// FSaveExtensionArchive


FArchive& FSaveExtensionArchive::operator<<(FSoftObjectPtr& Value)
{
	*this << Value.GetUniqueID();

	return *this;
}

FArchive& FSaveExtensionArchive::operator<<(FSoftObjectPath& Value)
{
	FString Path = Value.ToString();

	*this << Path;

	if (IsLoading())
	{
		Value.SetPath(MoveTemp(Path));
	}

	return *this;
}


/*FArchive& FSaveExtensionArchive::operator<<(FSaveTimerHandle& Value)
{
	*this << Value.bLooping << Value.Time << Value.Delegate;

	SE_LOG(true, "Serializing Timer");
	float Remaining = -1.f;

	const UObject* Obj = Value.Delegate.GetUObject();

	//Find World from the UObject
	const UWorld* World = Obj ? Obj->GetWorld() : nullptr;
	if (World)
	{
		Remaining = World->GetTimerManager().GetTimerRemaining(Value.Handle);
		SE_LOG(true, FString::Printf(TEXT("Saving Remaining Time: %f"), Remaining));
	}

	*this << Remaining;

	if (IsLoading() && World && Remaining > 0.f)
	{
		SE_LOG(true, "Restoring Timer", FColor::White, true);
		//Stop current timer if exists and start a new one
		World->GetTimerManager().SetTimer(Value.Handle, Value.Delegate, Value.bLooping ? Value.Time : Remaining, Value.bLooping, Value.bLooping ? Remaining : -1.f);
	}

	return *this;
}*/


/////////////////////////////////////////////////////
// USaveData

USlotData::USlotData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PlayerId = 0;
}

void USlotData::Clean(bool bKeepLevels)
{
	//Clean Up serialization data
	GameMode = {};
	GameState = {};

	PlayerPawn = {};
	PlayerController = {};
	PlayerState = {};
	PlayerHUD = {};

	GameInstance = {};

	MainLevel.Clean();

	if (!bKeepLevels)
	{
		SubLevels.Empty();
	}
}
