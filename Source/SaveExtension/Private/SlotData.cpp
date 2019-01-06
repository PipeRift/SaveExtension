// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SlotData.h"
#include <TimerManager.h>

#include "SavePreset.h"


/**
 * A macro for tidying up accessing of private members, through the above code
 *
 * @param InClass		The class being accessed (not a string, just the class, i.e. FStackTracker)
 * @param InObj			Pointer to an instance of the specified class
 * @param MemberName	Name of the member being accessed (again, not a string)
 * @return				The value of the member
 */
#define GET_PRIVATE(InClass, InObj, MemberName) (*InObj).*GetPrivate(InClass##MemberName##Accessor())


FName FPersistentLevelRecord::PersistentName{ "Persistent" };


/////////////////////////////////////////////////////
// Records

bool FBaseRecord::Serialize(FArchive& Ar)
{
	Ar << Name;
	Ar << Class;
	return true;
}

FObjectRecord::FObjectRecord(const UObject* Object) : Super()
{
	if (Object)
	{
		Name = Object->GetFName();
		Class = Object->GetClass();
	}
}

bool FObjectRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << Data;
	Ar << Tags;
	return true;
}

bool FComponentRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << Transform;
	return true;
}

bool FActorRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.SerializeBits(&bHiddenInGame, 1);
	Ar.SerializeBits(&bIsProcedural, 1);

	Ar << Transform;
	Ar << LinearVelocity;
	Ar << AngularVelocity;
	Ar << ComponentRecords;
	return true;
}

bool FControllerRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << ControlRotation;

	return true;
}


bool FLevelRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << LevelScript;
	Ar << Actors;
	Ar << AIControllers;

	return true;
}

void FLevelRecord::Clean()
{
	LevelScript = {};
	Actors.Empty();
	AIControllers.Empty();
}


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


/////////////////////////////////////////////////////
// USlotData

void USlotData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << GameInstance;
	Ar << GameMode;
	Ar << GameState;

	Ar << PlayerPawn;
	Ar << PlayerController;
	Ar << PlayerState;
	Ar << PlayerHUD;

	MainLevel.Serialize(Ar);
	Ar << SubLevels;
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
