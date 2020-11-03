// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "SaveManager.h"

#include "FileAdapter.h"
#include "LatentActions/LoadInfosAction.h"
#include "Multithreading/LoadSlotInfoTask.h"
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

	ActivePreset = GetDefault<USaveSettings>()->CreatePreset(this);

	// AutoLoad
	if (GetPreset() && GetPreset()->bAutoLoad)
	{
		ReloadCurrentSlot();
	}

	TryInstantiateInfo();
	UpdateLevelStreamings();

	AddToRoot();
}

void USaveManager::Deinitialize()
{
	Super::Deinitialize();

	MTTasks.CancelAll();

	if (GetPreset()->bSaveOnExit)
		SaveCurrentSlot();

	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);

	// Destroy
	RemoveFromRoot();
	MarkPendingKill();
}

bool USaveManager::SaveSlot(
	int32 SlotId, bool bOverrideIfNeeded, bool bScreenshot, const FScreenshotSize Size, FOnGameSaved OnSaved)
{
	if (!CanLoadOrSave())
		return false;

	const USavePreset* Preset = GetPreset();
	if (!IsValidSlot(SlotId))
	{
		SELog(Preset, "Invalid Slot. Cant go under 0 or exceed MaxSlots.", true);
		return false;
	}

	// Saving
	SELog(Preset, "Saving to Slot " + FString::FromInt(SlotId));

	UWorld* World = GetWorld();
	check(World);

	// Launch task, always fail if it didn't finish or wasn't scheduled
	auto* Task = CreateTask<USlotDataTask_Saver>()
					 ->Setup(SlotId, bOverrideIfNeeded, bScreenshot, Size.Width, Size.Height)
					 ->Bind(OnSaved)
					 ->Start();

	return Task->IsSucceeded() || Task->IsScheduled();
}

bool USaveManager::LoadSlot(int32 SlotId, FOnGameLoaded OnLoaded)
{
	if (!CanLoadOrSave() || !IsSlotSaved(SlotId))
	{
		return false;
	}

	TryInstantiateInfo();

	auto* Task = CreateTask<USlotDataTask_Loader>()->Setup(SlotId)->Bind(OnLoaded)->Start();

	return Task->IsSucceeded() || Task->IsScheduled();
}

bool USaveManager::DeleteSlot(int32 SlotId)
{
	if (!IsValidSlot(SlotId))
		return false;

	bool bSuccess = false;

	MTTasks.CreateTask<FDeleteSlotsTask>(this, SlotId)
		.OnFinished([&bSuccess](auto& Task) mutable {
			bSuccess = Task->bSuccess;
		})
		.StartSynchronousTask();
	MTTasks.Tick();
	return bSuccess;
}

void USaveManager::LoadAllSlotInfos(bool bSortByRecent, FOnAllInfosLoaded Delegate)
{
	MTTasks.CreateTask<FLoadAllSlotInfosTask>(this, bSortByRecent, MoveTemp(Delegate))
		.OnFinished([](auto& Task) {
			Task->CallDelegate();
		})
		.StartBackgroundTask();
}

void USaveManager::LoadAllSlotInfosSync(bool bSortByRecent, FOnAllInfosLoaded Delegate)
{
	MTTasks.CreateTask<FLoadAllSlotInfosTask>(this, bSortByRecent, MoveTemp(Delegate))
		.OnFinished([](auto& Task) {
			Task->CallDelegate();
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

void USaveManager::BPSaveSlotToId(int32 SlotId, bool bScreenshot, const FScreenshotSize Size,
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
				new FSaveGameAction(this, SlotId, bOverrideIfNeeded, bScreenshot, Size, Result, LatentInfo));
		}
		return;
	}
	Result = ESaveGameResult::Failed;
}

void USaveManager::BPLoadSlotFromId(
	int32 SlotId, ELoadGameResult& Result, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GetWorld())
	{
		Result = ELoadGameResult::Loading;

		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FLoadGameAction>(
				LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID,
				new FLoadGameAction(this, SlotId, Result, LatentInfo));
		}
		return;
	}
	Result = ELoadGameResult::Failed;
}

void USaveManager::BPLoadAllSlotInfos(const bool bSortByRecent, TArray<USlotInfo*>& SaveInfos,
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

bool USaveManager::IsSlotSaved(int32 SlotId) const
{
	if (!IsValidSlot(SlotId))
	{
		return false;
	}
	return FFileAdapter::DoesFileExist(GenerateSlotName(SlotId));
}

bool USaveManager::SetActivePreset(USavePreset* Preset)
{
	// We can only change a preset if we have no tasks running
	if (HasTasks() || !Preset || ActivePreset == Preset)
	{
		return false;
	}

	ActivePreset = Preset;
	return true;
}

const USavePreset* USaveManager::GetPreset() const
{
	if (IsValid(ActivePreset))
	{
		return ActivePreset;
	}
	return GetDefault<USavePreset>();
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

void USaveManager::TryInstantiateInfo(bool bForced)
{
	if (IsInSlot() && !bForced)
		return;

	const USavePreset* Preset = GetPreset();

	UClass* InfoTemplate = Preset->SlotInfoTemplate.Get();
	if (!InfoTemplate)
		InfoTemplate = USlotInfo::StaticClass();

	UClass* DataTemplate = Preset->SlotDataTemplate.Get();
	if (!DataTemplate)
		DataTemplate = USlotData::StaticClass();

	CurrentInfo = NewObject<USlotInfo>(GetTransientPackage(), InfoTemplate);
	CurrentData = NewObject<USlotData>(GetTransientPackage(), DataTemplate);
}

void USaveManager::UpdateLevelStreamings()
{
	UWorld* World = GetWorld();
	check(World);

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
		CreateTask<USlotDataTask_LevelSaver>()->Setup(LevelStreaming)->Start();
	}
}

void USaveManager::DeserializeStreamingLevel(ULevelStreaming* LevelStreaming)
{
	CreateTask<USlotDataTask_LevelLoader>()->Setup(LevelStreaming)->Start();
}

USlotInfo* USaveManager::LoadInfo(uint32 SlotId) const
{
	if (!IsValidSlot(SlotId))
	{
		SELog(GetPreset(), "Invalid Slot. Cant go under 0 or exceed MaxSlots", true);
		return nullptr;
	}

	FAsyncTask<FLoadSlotInfoTask>* LoadInfoTask = new FAsyncTask<FLoadSlotInfoTask>(this, SlotId);
	LoadInfoTask->StartSynchronousTask();

	check(LoadInfoTask->IsDone());

	return LoadInfoTask->GetTask().GetLoadedSlot();
}

USlotData* USaveManager::LoadData(const USlotInfo* InSaveInfo) const
{
	if (!InSaveInfo)
	{
		return nullptr;
	}

	const FString Card = GenerateSlotName(InSaveInfo->Id);

	USlotInfo* LoadedInfo = nullptr;
	USlotData* LoadedData = nullptr;
	if(FFileAdapter::LoadFile(Card, LoadedInfo, LoadedData, true))
	{
		return LoadedData;
	}
	return nullptr;
}

USlotDataTask* USaveManager::CreateTask(TSubclassOf<USlotDataTask> TaskType)
{
	USlotDataTask* Task = NewObject<USlotDataTask>(this, TaskType.Get());
	Task->Prepare(CurrentData, *GetPreset());
	Tasks.Add(Task);
	return Task;
}

void USaveManager::FinishTask(USlotDataTask* Task)
{
	Tasks.Remove(Task);

	// Start next task
	if (Tasks.Num() > 0)
	{
		Tasks[0]->Start();
	}
}

bool USaveManager::IsLoading() const
{
	return HasTasks() &&
		   (Tasks[0]->IsA<USlotDataTask_Loader>() || Tasks[0]->IsA<USlotDataTask_LevelLoader>());
}

void USaveManager::Tick(float DeltaTime)
{
	if (Tasks.Num())
	{
		USlotDataTask* Task = Tasks[0];
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

void USaveManager::OnSaveBegan(const FSaveFilter& Filter)
{
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

void USaveManager::OnSaveFinished(const FSaveFilter& Filter, const bool bError)
{
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

void USaveManager::OnLoadBegan(const FSaveFilter& Filter)
{
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

void USaveManager::OnLoadFinished(const FSaveFilter& Filter, const bool bError)
{
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
	SELog(GetPreset(), "Loading Map '" + MapName + "'", FColor::Purple);
}

void USaveManager::OnMapLoadFinished(UWorld* LoadedWorld)
{
	if(auto* ActiveLoader = Cast<USlotDataTask_Loader>(Tasks.Num() ? Tasks[0] : nullptr))
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
