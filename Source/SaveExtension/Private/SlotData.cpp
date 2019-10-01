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

/////////////////////////////////////////////////////
// USlotData

void USlotData::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar << GameInstance;

	MainLevel.Serialize(Ar);
	Ar << SubLevels;
}

void USlotData::Clean(bool bKeepLevels)
{
	//Clean Up serialization data
	GameInstance = {};

	MainLevel.Clean();

	if (!bKeepLevels)
	{
		SubLevels.Empty();
	}
}
