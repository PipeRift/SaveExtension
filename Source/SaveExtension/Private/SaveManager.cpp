// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveManager.h"

#include "FileAdapter.h"
#include "LatentActions/LoadInfosAction.h"
#include "Multithreading/DeleteSlotsTask.h"
#include "Multithreading/LoadSlotInfosTask.h"
#include "SaveSettings.h"
#include "Serialization/SlotDataTask_LevelLoader.h"
#include "Serialization/SlotDataTask_LevelSaver.h"
#include "Serialization/SlotDataTask_Loader.h"
#include "Serialization/SlotDataTask_Saver.h"

#include <Engine/GameViewportClient.h>
#include <Engine/LevelStreaming.h>
#include <EngineUtils.h>
#include <GameDelegates.h>
#include <GameFramework/GameModeBase.h>
#include <HighResScreenshot.h>
#include <Kismet/GameplayStatics.h>
#include <Misc/CoreDelegates.h>
#include <Misc/Paths.h>


USaveManager::USaveManager() : Super(), MTTasks{} {}

void USaveManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bTickWithGameWorld = GetDefault<USaveSettings>()->bTickWithGameWorld;

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &USaveManager::OnMapLoadStarted);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USaveManager::OnMapLoadFinished);

	AssureActiveSlot();
	if (ActiveSlot && ActiveSlot->bLoadOnStart)
	{
		ReloadCurrentSlot();
	}

	UpdateLevelStreamings();
}

void USaveManager::Deinitialize()
{
	Super::Deinitialize();

	MTTasks.CancelAll();

	if (GetActivePreset()->bSaveOnExit)
		SaveCurrentSlot();

	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);
}

bool USaveManager::SaveSlot(FName SlotName, bool bOverrideIfNeeded, bool bScreenshot,
	const FScreenshotSize Size, FOnGameSaved OnSaved)
{
	if (!CanLoadOrSave())
		return false;

	const USavePreset* Preset = GetActivePreset();
	if (SlotName.IsNone())
	{
		SELog(Preset, "Can't use an empty slot name to save.", true);
		return false;
	}

	// Saving
	SELog(Preset, "Saving to Slot " + SlotName.ToString());

	UWorld* World = GetWorld();
	check(World);

	// Launch task, always fail if it didn't finish or wasn't scheduled
	auto* Task = CreateTask<USaveSlotDataTask_Saver>()
					 ->Setup(SlotName, bOverrideIfNeeded, bScreenshot, Size.Width, Size.Height)
					 ->Bind(OnSaved)
					 ->Start();

	return Task->IsSucceeded() || Task->IsScheduled();
}

bool USaveManager::LoadSlot(FName SlotName, FOnGameLoaded OnLoaded)
{
	if (!CanLoadOrSave() || !IsSlotSaved(SlotName))
	{
		return false;
	}

	AssureActiveSlot();

	auto* Task = CreateTask<USaveSlotDataTask_Loader>()->Setup(SlotName)->Bind(OnLoaded)->Start();

	return Task->IsSucceeded() || Task->IsScheduled();
}

bool USaveManager::DeleteSlot(FName SlotName)
{
	if (SlotName.IsNone())
	{
		return false;
	}

	bool bSuccess = false;
	MTTasks.CreateTask<FDeleteSlotsTask>(this, SlotName)
		.OnFinished([&bSuccess](auto& Task) mutable {
			bSuccess = Task->bSuccess;
		})
		.StartSynchronousTask();
	MTTasks.Tick();
	return bSuccess;
}

void USaveManager::FindAllSlots(bool bSortByRecent, FOnSlotInfosLoaded Delegate)
{
	MTTasks.CreateTask<FLoadSlotInfosTask>(this, bSortByRecent, MoveTemp(Delegate))
		.OnFinished([](auto& Task) {
			Task->AfterFinish();
		})
		.StartBackgroundTask();
}

void USaveManager::FindAllSlotsSync(bool bSortByRecent, TArray<USaveSlot*>& Slots)
{
	auto Delegate = [](const TArray<class USaveSlot*>& FoundSlots) {
		Slots = FoundSlots;
	};
	MTTasks.CreateTask<FLoadSlotInfosTask>(this, bSortByRecent, Delegate)
		.OnFinished([](auto& Task) {
			Task->AfterFinish();
		})
		.StartSynchronousTask();
	MTTasks.Tick();
}

void USaveManager::DeleteAllSlots(FOnSlotsDeleted Delegate)
{
	MTTasks.CreateTask<FDeleteSlotsTask>(this)
		.OnFinished([Delegate](auto& Task) {
			Delegate.ExecuteIfBound();
		})
		.StartBackgroundTask();
}

void USaveManager::BPSaveSlot(FName SlotName, bool bScreenshot, const FScreenshotSize Size,
	ESaveGameResult& Result, struct FLatentActionInfo LatentInfo, bool bOverrideIfNeeded /*= true*/)
{
	if (UWorld* World = GetWorld())
	{
		Result = ESaveGameResult::Saving;

		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FSaveGameAction>(
				LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID,
				new FSaveGameAction(
					this, SlotName, bOverrideIfNeeded, bScreenshot, Size, Result, LatentInfo));
		}
		return;
	}
	Result = ESaveGameResult::Failed;
}

void USaveManager::BPLoadSlot(FName SlotName, ELoadGameResult& Result, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GetWorld())
	{
		Result = ELoadGameResult::Loading;

		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FLoadGameAction>(
				LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID,
				new FLoadGameAction(this, SlotName, Result, LatentInfo));
		}
		return;
	}
	Result = ELoadGameResult::Failed;
}

void USaveManager::BPFindAllSlots(const bool bSortByRecent, TArray<USaveSlot*>& SaveInfos,
	ELoadInfoResult& Result, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GetWorld())
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FLoadInfosAction>(
				LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID,
				new FLoadInfosAction(this, bSortByRecent, SaveInfos, Result, LatentInfo));
		}
	}
}

void USaveManager::BPDeleteAllSlots(EDeleteSlotsResult& Result, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GetWorld())
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FDeleteSlotsAction>(
				LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			LatentActionManager.AddNewAction(
				LatentInfo.CallbackTarget, LatentInfo.UUID, new FDeleteSlotsAction(this, Result, LatentInfo));
		}
	}
}

bool USaveManager::IsSlotSaved(FName SlotName) const
{
	return FFileAdapter::FileExists(SlotName.ToString());
}

bool USaveManager::CanLoadOrSave()
{
	const AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this);
	if (GameMode && !GameMode->HasAuthority())
	{
		return false;
	}

	return IsValid(GetWorld());
}

void USaveManager::AssureActiveSlot(bool bForced)
{
	if (IsInSlot() && !bForced)
		return;

	UClass* ActiveSlotClass = GetDefault<USaveSettings>()->ActiveSlot.Get();
	if (!ActiveSlotClass)
	{
		ActiveSlotClass = USaveSlot::StaticClass();
	}
	ActiveSlot = NewObject<USaveSlot>(this, ActiveSlotClass);
}

void USaveManager::UpdateLevelStreamings()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const TArray<ULevelStreaming*>& Levels = World->GetStreamingLevels();

	LevelStreamingNotifiers.Empty(Levels.Num());	  // Avoid memory deallocation
	LevelStreamingNotifiers.Reserve(Levels.Num());	  // Reserve extra memory
	for (auto* Level : Levels)
	{
		ULevelStreamingNotifier* Notifier = NewObject<ULevelStreamingNotifier>(this);
		Notifier->SetLevelStreaming(Level);
		Notifier->OnLevelShown().BindUFunction(
			this, GET_FUNCTION_NAME_CHECKED(USaveManager, DeserializeStreamingLevel));
		Notifier->OnLevelHidden().BindUFunction(
			this, GET_FUNCTION_NAME_CHECKED(USaveManager, SerializeStreamingLevel));
		LevelStreamingNotifiers.Add(Notifier);
	}
}

void USaveManager::SerializeStreamingLevel(ULevelStreaming* LevelStreaming)
{
	if (!LevelStreaming->GetLoadedLevel()->bIsBeingRemoved)
	{
		CreateTask<USaveSlotDataTask_LevelSaver>()->Setup(LevelStreaming)->Start();
	}
}

void USaveManager::DeserializeStreamingLevel(ULevelStreaming* LevelStreaming)
{
	CreateTask<USaveSlotDataTask_LevelLoader>()->Setup(LevelStreaming)->Start();
}

USaveSlot* USaveManager::LoadInfo(FName SlotName)
{
	if (SlotName.IsNone())
	{
		SELog(GetActivePreset(), "Invalid Slot. Cant go under 0 or exceed MaxSlots", true);
		return nullptr;
	}

	auto& Task = MTTasks.CreateTask<FLoadSlotInfosTask>(this, SlotName).OnFinished([](auto& Task) {
		Task->AfterFinish();
	});
	Task.StartSynchronousTask();

	check(Task.IsDone());

	const auto& Infos = Task->GetLoadedSlots();
	return Infos.Num() > 0 ? Infos[0] : nullptr;
}

USaveSlotDataTask* USaveManager::CreateTask(TSubclassOf<USaveSlotDataTask> TaskType)
{
	USaveSlotDataTask* Task = NewObject<USaveSlotDataTask>(this, TaskType.Get());
	Task->Prepare(CurrentInfo->GetData(), *GetActivePreset());
	Tasks.Add(Task);
	return Task;
}

void USaveManager::FinishTask(USaveSlotDataTask* Task)
{
	Tasks.Remove(Task);

	// Start next task
	if (Tasks.Num() > 0)
	{
		Tasks[0]->Start();
	}
}

FName USaveManager::GetSlotNameFromId(const int32 SlotId) const
{
	if (const auto* Preset = GetActivePreset())
	{
		FName Name;
		Preset->BPGetSlotNameFromId(SlotId, Name);
		return Name;
	}
	return FName{FString::FromInt(SlotId)};
}

bool USaveManager::IsLoading() const
{
	return HasTasks() &&
		   (Tasks[0]->IsA<USaveSlotDataTask_Loader>() || Tasks[0]->IsA<USaveSlotDataTask_LevelLoader>());
}

void USaveManager::Tick(float DeltaTime)
{
	if (Tasks.Num())
	{
		USaveSlotDataTask* Task = Tasks[0];
		check(Task);
		if (Task->IsRunning())
		{
			Task->Tick(DeltaTime);
		}
	}

	MTTasks.Tick();
}

void USaveManager::SubscribeForEvents(const TScriptInterface<ISaveExtensionInterface>& Interface)
{
	SubscribedInterfaces.AddUnique(Interface);
}

void USaveManager::UnsubscribeFromEvents(const TScriptInterface<ISaveExtensionInterface>& Interface)
{
	SubscribedInterfaces.Remove(Interface);
}

void USaveManager::OnSaveBegan(const FSELevelFilter& Filter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USaveManager::OnSaveBegan);

	IterateSubscribedInterfaces([&Filter](auto* Object) {
		check(Object->template Implements<USaveExtensionInterface>());

		// C++ event
		if (ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object))
		{
			Interface->OnSaveBegan(Filter);
		}
		ISaveExtensionInterface::Execute_ReceiveOnSaveBegan(Object, Filter);
	});
}

void USaveManager::OnSaveFinished(const FSELevelFilter& Filter, const bool bError)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USaveManager::OnSaveFinished);

	IterateSubscribedInterfaces([&Filter, bError](auto* Object) {
		check(Object->template Implements<USaveExtensionInterface>());

		// C++ event
		if (ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object))
		{
			Interface->OnSaveFinished(Filter, bError);
		}
		ISaveExtensionInterface::Execute_ReceiveOnSaveFinished(Object, Filter, bError);
	});

	if (!bError)
	{
		OnGameSaved.Broadcast(CurrentInfo);
	}
}

void USaveManager::OnLoadBegan(const FSELevelFilter& Filter)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USaveManager::OnLoadBegan);

	IterateSubscribedInterfaces([&Filter](auto* Object) {
		check(Object->template Implements<USaveExtensionInterface>());

		// C++ event
		if (ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object))
		{
			Interface->OnLoadBegan(Filter);
		}
		ISaveExtensionInterface::Execute_ReceiveOnLoadBegan(Object, Filter);
	});
}

void USaveManager::OnLoadFinished(const FSELevelFilter& Filter, const bool bError)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USaveManager::OnLoadFinished);

	IterateSubscribedInterfaces([&Filter, bError](auto* Object) {
		check(Object->template Implements<USaveExtensionInterface>());

		// C++ event
		if (ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object))
		{
			Interface->OnLoadFinished(Filter, bError);
		}
		ISaveExtensionInterface::Execute_ReceiveOnLoadFinished(Object, Filter, bError);
	});

	if (!bError)
	{
		OnGameLoaded.Broadcast(CurrentInfo);
	}
}

void USaveManager::OnMapLoadStarted(const FString& MapName)
{
	SELog(GetActivePreset(), "Loading Map '" + MapName + "'", FColor::Purple);
}

void USaveManager::OnMapLoadFinished(UWorld* LoadedWorld)
{
	if (auto* ActiveLoader = Cast<USaveSlotDataTask_Loader>(Tasks.Num() ? Tasks[0] : nullptr))
	{
		ActiveLoader->OnMapLoaded();
	}

	UpdateLevelStreamings();
}

UWorld* USaveManager::GetWorld() const
{
	check(GetGameInstance());

	// If we are a CDO, we must return nullptr instead to fool UObject::ImplementsGetWorld.
	if (HasAllFlags(RF_ClassDefaultObject))
		return nullptr;

	return GetGameInstance()->GetWorld();
}
