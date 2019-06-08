// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SEArchive.h"
#include <TimerManager.h>


/////////////////////////////////////////////////////
// FSaveExtensionArchive

FArchive& FSaveExtensionArchive::operator<<(UObject*& Obj)
{
	if (IsLoading())
	{
		// Deserialize the path name to the object
		FString ObjectPath;
		InnerArchive << ObjectPath;

		if (ObjectPath.IsEmpty())
		{
			// No object to deserialize
			Obj = nullptr;
			return *this;
		}

		// Only serialize owned Objects
		bool bIsLocallyOwned;
		InnerArchive << bIsLocallyOwned;
		if (bIsLocallyOwned)
		{
			if (!Obj)
			{
				//#TODO: Create Object from Serialized Data
				//Obj = NewObject<UObject>(FindOwner, ObjectPath);
			}
			Obj->Serialize(*this);
		}
		else
		{
			// If Objects are not owned, maybe they are an asset (or something similar)

			// Look up the object by fully qualified pathname
			Obj = FindObject<UObject>(nullptr, *ObjectPath, false);
			// If we couldn't find it, and we want to load it, do that
			if (!Obj && bLoadIfFindFails)
			{
				Obj = LoadObject<UObject>(nullptr, *ObjectPath);
			}
		}
	}
	else
	{
		// Serialize the fully qualified object name
		FString SavedString(Obj? Obj->GetPathName() : "");
		InnerArchive << SavedString;

		if (Obj)
		{
			// Only serialize owned Objects
			bool bIsLocallyOwned = IsObjectOwned(Obj);
			InnerArchive << bIsLocallyOwned;
			if (bIsLocallyOwned)
			{
				Obj->Serialize(*this);
			}
		}
	}
	return *this;
}

bool FSaveExtensionArchive::IsObjectOwned(const UObject* Obj) const
{
	// Find if this object is (directly or indirectly) owned by the rootOuter
	const UObject* Outer = Obj->GetOuter();
	while (Outer)
	{
		if (Outer == rootOuter)
		{
			return true;
		}
		Outer = Outer->GetOuter();
	}
	return false;
}
