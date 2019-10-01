// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Serialization/SEArchive.h"


/////////////////////////////////////////////////////
// FSEArchive

FArchive& FSEArchive::operator<<(UObject*& Obj)
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

		// #FIX: Deserialize and assign outers

		// Look up the object by fully qualified pathname
		Obj = FindObject<UObject>(nullptr, *ObjectPath, false);
		// If we couldn't find it, and we want to load it, do that
		if (!Obj && bLoadIfFindFails)
		{
			Obj = LoadObject<UObject>(nullptr, *ObjectPath);
		}

		// Only serialize owned Objects
		/*bool bIsLocallyOwned;
		InnerArchive << bIsLocallyOwned;
		if (Obj && bIsLocallyOwned)
		{
			Obj->Serialize(*this);
		}*/
	}
	else
	{
		if (Obj)
		{
			// Serialize the fully qualified object name
			FString SavedString{ Obj->GetPathName() };
			InnerArchive << SavedString;

			/*bool bIsLocallyOwned = IsObjectOwned(Obj);
			InnerArchive << bIsLocallyOwned;
			if (bIsLocallyOwned)
			{
				Obj->Serialize(*this);
			}*/
		}
		else
		{
			FString SavedString{ "" };
			InnerArchive << SavedString;

			/*bool bIsLocallyOwned = false;
			InnerArchive << bIsLocallyOwned;*/
		}
	}
	return *this;
}
