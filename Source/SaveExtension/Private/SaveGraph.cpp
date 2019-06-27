// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SaveGraph.h"
#include "SaveManager.h"

#include "SlotInfo.h"
#include "SlotData.h"


bool USaveGraph::DoPrepare(USlotInfo* Info, USlotData* Data, TArray<FLevelRecord*>&& InSavedSubLevels)
{
	SlotInfo = Info;
	SlotData = Data;
	SavedSubLevels = MoveTemp(InSavedSubLevels);
	return EventPrepare();
}

bool USaveGraph::EventPrepare_Implementation()
{
	return Prepare();
}

void USaveGraph::AddActorPacket(const FActorClassFilter& Filter, const FActorPacketSettings& Settings/*, TSubclassOf<UActorSerializer> CustomSerializer*/)
{
	// Add packet to each valid level based on settings and if it doesn't contain it already
	FActorPacketRecord NewPacket{ Filter };

	switch (Settings.Levels)
	{
	case EPacketLevelMode::RootLevelOnly:
		SlotData->MainLevel.FindOrCreateActorPacket(NewPacket);
		break;

	case EPacketLevelMode::AllLevels:
		SlotData->MainLevel.FindOrCreateActorPacket(NewPacket);

	case EPacketLevelMode::SubLevelsOnly:
		for (auto* SubLevel : SavedSubLevels)
		{
			SubLevel->FindOrCreateActorPacket(NewPacket);
		}
		break;

	case EPacketLevelMode::SpecifiedLevels:
		if (Settings.IsLevelAllowed(SlotData->MainLevel.Name))
		{
			SlotData->MainLevel.FindOrCreateActorPacket(NewPacket);
		}

		for (auto* SubLevel : SavedSubLevels)
		{
			if (Settings.IsLevelAllowed(SubLevel->Name))
			{
				SubLevel->FindOrCreateActorPacket(NewPacket);
			}
		}
		break;
	}
}

UWorld* USaveGraph::GetWorld() const
{
	// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
	if (HasAllFlags(RF_ClassDefaultObject))
		return nullptr;

	return GetOuter()->GetWorld();
}

class USaveManager* USaveGraph::GetManager() const
{
	return Cast<USaveManager>(GetOuter());
}
