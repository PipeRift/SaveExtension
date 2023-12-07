// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Serialization/Records.h"

#include "ClassFilter.h"
#include "SaveExtension.h"
#include "SaveSlotData.h"
#include "Serialization/SEArchive.h"

#include <Components/PrimitiveComponent.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/PlayerState.h>


/////////////////////////////////////////////////////
// Records

bool FBaseRecord::Serialize(FArchive& Ar)
{
	Ar << Name;
	return true;
}

FObjectRecord::FObjectRecord(const UObject* Object) : Super()
{
	Class = nullptr;
	if (Object)
	{
		Name = Object->GetFName();
		Class = Object->GetClass();
	}
}

bool FObjectRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (!Name.IsNone())
		Ar << Class;
	else if (Ar.IsLoading())
		Class = nullptr;

	if (Class)
	{
		Ar << Data;
		Ar << Tags;
	}
	return true;
}

bool FComponentRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Class)
	{
		Ar << Transform;
	}
	return true;
}

bool FActorRecord::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (!Class)
	{
		return true;
	}

	Ar.SerializeBits(&bHiddenInGame, 1);
	Ar.SerializeBits(&bIsProcedural, 1);

	Ar << Transform;

	// Reduce memory footprint to 1 bool if not moving
	bool bIsMoving = Ar.IsSaving() && (!LinearVelocity.IsNearlyZero() || !AngularVelocity.IsNearlyZero());
	Ar << bIsMoving;
	if (bIsMoving)
	{
		Ar << LinearVelocity;
		Ar << AngularVelocity;
	}
	Ar << ComponentRecords;
	return true;
}


FSubsystemRecord::FSubsystemRecord(const USubsystem* Subsystem) : Super(Subsystem) {}


bool FPlayerRecord::operator==(const FPlayerRecord& Other) const
{
	return UniqueId == Other.UniqueId;
}


const FName SERecords::TagNoTransform{"!SaveTransform"};
const FName SERecords::TagNoPhysics{"!SavePhysics"};
const FName SERecords::TagNoTags{"!SaveTags"};


void SERecords::SerializeActor(
	const AActor* Actor, FActorRecord& Record, const FSEClassFilter& ComponentFilter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(SerializeActor);

	Record = FActorRecord{Actor};

	Record.bHiddenInGame = Actor->IsHidden();
	Record.bIsProcedural = Actor->HasAnyFlags(RF_WasLoaded | RF_LoadCompleted);

	if (StoresTags(Actor))
	{
		Record.Tags = Actor->Tags;
	}
	else	// only save save-tags
	{
		for (const auto& Tag : Actor->Tags)
		{
			if (IsSaveTag(Tag))
			{
				Record.Tags.Add(Tag);
			}
		}
	}

	if (StoresTransform(Actor))
	{
		Record.Transform = Actor->GetTransform();
		if (StoresPhysics(Actor))
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

	if (ComponentFilter.IsAnyAllowed())
	{
		for (auto* Component : Actor->GetComponents())
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(SerializeActor | Component);
			if (IsValid(Component) && ComponentFilter.IsAllowed(Component->GetClass()))
			{
				FComponentRecord& ComponentRecord = Record.ComponentRecords.Add_GetRef({Component});
				if (const auto* SceneComp = Cast<USceneComponent>(Component))
				{
					if (SceneComp->Mobility == EComponentMobility::Movable)
					{
						ComponentRecord.Transform = SceneComp->GetRelativeTransform();
					}
				}

				if (StoresTags(Component))
				{
					ComponentRecord.Tags = Component->ComponentTags;
				}

				if (Component->GetClass()->IsChildOf<UPrimitiveComponent>())
				{
					continue;
				}

				FMemoryWriter MemoryWriter(ComponentRecord.Data, true);
				FSEArchive Archive(MemoryWriter, false);
				Component->Serialize(Archive);
			}
		}
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(SerializeActor | Serialize);
	FMemoryWriter MemoryWriter(Record.Data, true);
	FSEArchive Archive(MemoryWriter, false);
	const_cast<AActor*>(Actor)->Serialize(Archive);
}

bool SERecords::DeserializeActor(
	AActor* Actor, const FActorRecord& Record, const FSEClassFilter& ComponentFilter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(DeserializeActor);

	if (Actor->GetClass() != Record.Class)
	{
		UE_LOG(LogSaveExtension, Log, TEXT("Actor '{}' exists but class doesn't match"), Record.Name);
		return false;
	}

	Actor->Tags = Record.Tags;

	if (StoresTransform(Actor))
	{
		Actor->SetActorTransform(Record.Transform);
		if (StoresPhysics(Actor))
		{
			USceneComponent* Root = Actor->GetRootComponent();
			if (auto* Primitive = Cast<UPrimitiveComponent>(Root))
			{
				Primitive->SetPhysicsLinearVelocity(Record.LinearVelocity);
				Primitive->SetPhysicsAngularVelocityInRadians(Record.AngularVelocity);
			}
			else
			{
				Root->ComponentVelocity = Record.LinearVelocity;
			}
		}
	}

	Actor->SetActorHiddenInGame(Record.bHiddenInGame);

	TRACE_CPUPROFILER_EVENT_SCOPE(UFSEDataTask_Load::DeserializeActorComponents);

	if (ComponentFilter.IsAnyAllowed())
	{
		for (auto* Component : Actor->GetComponents())
		{
			TRACE_CPUPROFILER_EVENT_SCOPE(DeserializeActor | Component);
			if (!IsValid(Component) || !ComponentFilter.IsAllowed(Component->GetClass()))
			{
				continue;
			}

			// Find the record
			const FComponentRecord* ComponentRecord = Record.ComponentRecords.FindByKey(Component);
			if (!ComponentRecord)
			{
				continue;	 // Record not found.
			}

			if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
			{
				if (SceneComp->Mobility == EComponentMobility::Movable)
				{
					SceneComp->SetRelativeTransform(ComponentRecord->Transform);
				}
			}

			Component->ComponentTags = ComponentRecord->Tags;

			if (!Component->GetClass()->IsChildOf<UPrimitiveComponent>())
			{
				FMemoryReader MemoryReader(ComponentRecord->Data, true);
				FSEArchive Archive(MemoryReader, false);
				Component->Serialize(Archive);
			}
		}
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(DeserializeActor | Deserialize);
	FMemoryReader MemoryReader(Record.Data, true);
	FSEArchive Archive(MemoryReader, false);
	Actor->Serialize(Archive);
	return true;
}

void SERecords::SerializePlayer(
	const APlayerState* PlayerState, FPlayerRecord& Record, const FSEClassFilter& ComponentFilter)
{
	check(PlayerState);

	APlayerController* PC = PlayerState->GetPlayerController();
	APawn* Pawn = PlayerState->GetPawn();

	Record.UniqueId = PlayerState->GetUniqueId();
	SERecords::SerializeActor(PlayerState, Record.PlayerState, ComponentFilter);
	if (Pawn)
	{
		SERecords::SerializeActor(Pawn, Record.Pawn, ComponentFilter);
	}
	if (PC)
	{
		SERecords::SerializeActor(PC, Record.Controller, ComponentFilter);
	}
}

void SERecords::DeserializePlayer(
	APlayerState* PlayerState, const FPlayerRecord& Record, const FSEClassFilter& ComponentFilter)
{
	check(PlayerState);
	check(PlayerState->GetUniqueId() == Record.UniqueId);

	APlayerController* PC = PlayerState->GetPlayerController();
	APawn* Pawn = PlayerState->GetPawn();

	SERecords::DeserializeActor(PlayerState, Record.PlayerState, ComponentFilter);
	if (Pawn)
	{
		SERecords::DeserializeActor(Pawn, Record.Pawn, ComponentFilter);
	}
	if (PC)
	{
		SERecords::DeserializeActor(PC, Record.Controller, ComponentFilter);
	}
}


bool SERecords::IsSaveTag(const FName& Tag)
{
	return Tag == TagNoTransform || Tag == TagNoPhysics || Tag == TagNoTags;
}

bool SERecords::StoresTransform(const AActor* Actor)
{
	return Actor->IsRootComponentMovable() && !Actor->ActorHasTag(TagNoTransform);
}

bool SERecords::StoresPhysics(const AActor* Actor)
{
	return !Actor->ActorHasTag(TagNoPhysics);
}

bool SERecords::StoresTags(const AActor* Actor)
{
	return !Actor->ActorHasTag(TagNoTags);
}

bool SERecords::IsProcedural(const AActor* Actor)
{
	return Actor->HasAnyFlags(RF_WasLoaded | RF_LoadCompleted);
}

bool SERecords::StoresTags(const UActorComponent* Component)
{
	return !Component->ComponentHasTag(TagNoTags);
}
