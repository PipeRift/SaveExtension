// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "Serialization/MTTask_SerializeActors.h"
#include <Serialization/MemoryWriter.h>
#include <Components/PrimitiveComponent.h>

#include "SaveManager.h"
#include "SlotInfo.h"
#include "SlotData.h"
#include "SavePreset.h"
#include "Serialization/SEArchive.h"


/////////////////////////////////////////////////////
// FMTTask_SerializeActors
void FMTTask_SerializeActors::DoWork()
{
	if (Filter.bStoreGameInstance)
	{
		SerializeGameInstance();
	}

	for (int32 I = 0; I < Num; ++I)
	{
		const AActor* const Actor = (*LevelActors)[StartIndex + I];
		if (Actor && Filter.ShouldSave(Actor))
		{
			FActorRecord Record;
			SerializeActor(Actor, Record);
			ActorRecords.Add(MoveTemp(Record));
		}
	}
}

void FMTTask_SerializeActors::SerializeGameInstance()
{
	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		FObjectRecord Record{ GameInstance };

		//Serialize into Record Data
		FMemoryWriter MemoryWriter(Record.Data, true);
		FSEArchive Archive(MemoryWriter, false);
		GameInstance->Serialize(Archive);

		SlotData->GameInstance = MoveTemp(Record);
	}
}

bool FMTTask_SerializeActors::SerializeActor(const AActor* Actor, FActorRecord& Record) const
{
	//Clean the record
	Record = { Actor };

	Record.bHiddenInGame = Actor->IsHidden();
	Record.bIsProcedural = Filter.IsProcedural(Actor);

	if (Filter.StoresTags(Actor))
	{
		Record.Tags = Actor->Tags;
	}
	else
	{
		// Only save save-tags
		for (const auto& Tag : Actor->Tags)
		{
			if (Filter.IsSaveTag(Tag))
			{
				Record.Tags.Add(Tag);
			}
		}
	}

	if (Filter.StoresTransform(Actor))
	{
		Record.Transform = Actor->GetTransform();

		if (Filter.StoresPhysics(Actor))
		{
			USceneComponent* const Root = Actor->GetRootComponent();
			if (Root && Root->Mobility == EComponentMobility::Movable)
			{
				if (auto* const Primitive = Cast<UPrimitiveComponent>(Root))
				{
					Record.LinearVelocity = Primitive->GetPhysicsLinearVelocity();
					Record.AngularVelocity = Primitive->GetPhysicsAngularVelocityInRadians();
				}
				else
				{
					Record.LinearVelocity = Root->GetComponentVelocity();
				}
			}
		}
	}

	if (Filter.bStoreComponents)
	{
		SerializeActorComponents(Actor, Record, 1);
	}

	FMemoryWriter MemoryWriter(Record.Data, true);
	FSEArchive Archive(MemoryWriter, false);
	const_cast<AActor*>(Actor)->Serialize(Archive);

	return true;
}

void FMTTask_SerializeActors::SerializeActorComponents(const AActor* Actor, FActorRecord& ActorRecord, int8 Indent /*= 0*/) const
{
	const TSet<UActorComponent*>& Components = Actor->GetComponents();
	for (auto* Component : Components)
	{
		if (Filter.ShouldSave(Component))
		{
			FComponentRecord ComponentRecord;
			ComponentRecord.Name = Component->GetFName();
			ComponentRecord.Class = Component->GetClass();

			if (Filter.StoresTransform(Component))
			{
				const USceneComponent* Scene = CastChecked<USceneComponent>(Component);
				if (Scene->Mobility == EComponentMobility::Movable)
				{
					ComponentRecord.Transform = Scene->GetRelativeTransform();
				}
			}

			if (Filter.StoresTags(Component))
			{
				ComponentRecord.Tags = Component->ComponentTags;
			}

			if (!Component->GetClass()->IsChildOf<UPrimitiveComponent>())
			{
				FMemoryWriter MemoryWriter(ComponentRecord.Data, true);
				FSEArchive Archive(MemoryWriter, false);
				Component->Serialize(Archive);
			}
			ActorRecord.ComponentRecords.Add(ComponentRecord);
		}
	}
}
