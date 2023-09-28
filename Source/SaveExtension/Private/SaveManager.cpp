// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveManager.h"

#include "Multithreading/DeleteSlotsTask.h"
#include "Multithreading/LoadSlotsTask.h"
#include "SaveFileHelpers.h"
#include "SaveSettings.h"
#include "Serialization/SEDataTask_LoadLevel.h"
#include "Serialization/SEDataTask_SaveLevel.h"
#include "Serialization/SEDataTask_Load.h"
#include "Serialization/SEDataTask_Save.h"

#include <Engine/GameViewportClient.h>
#include <Engine/LatentActionManager.h>
#include <Engine/LevelStreaming.h>
#include <EngineUtils.h>
#include <GameDelegates.h>
#include <GameFramework/GameModeBase.h>
#include <HighResScreenshot.h>
#include <Kismet/GameplayStatics.h>
#include <LatentActions.h>
#include <Misc/CoreDelegates.h>
#include <Misc/Paths.h>


// BEGIN Async Actions

class FSELoadSlotDataAction : public FPendingLatentAction
{
public:
	ESEContinueOrFail& Result;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	FSELoadSlotDataAction(USaveManager* Manager, FName SlotName, ESEContinueOrFail& OutResult,
		const FLatentActionInfo& LatentInfo)
		: Result(OutResult)
		, ExecutionFunction(LatentInfo.ExecutionFunction)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
	{
		const bool bStarted = Manager->LoadSlot(
			SlotName, FOnGameLoaded::CreateRaw(this, &FSELoadSlotDataAction::OnLoadFinished));
		Result = bStarted ? ESEContinueOrFail::InProgress : ESEContinueOrFail::Failed;
	}
	void UpdateOperation(FLatentResponse& Response) override
	{
		Response.FinishAndTriggerIf(
			Result != ESEContinueOrFail::InProgress, ExecutionFunction, OutputLink, CallbackTarget);
	}
	void OnLoadFinished(USaveSlot* SavedSlot)
	{
		Result = SavedSlot ? ESEContinueOrFail::Continue : ESEContinueOrFail::Failed;
	}
#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	FString GetDescription() const override
	{
		return TEXT("Loading Game...");
	}
#endif
};


class FDeleteSlotsAction : public FPendingLatentAction
{
public:
	ESEContinue& Result;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	FDeleteSlotsAction(USaveManager* Manager, ESEContinue& OutResult, const FLatentActionInfo& LatentInfo)
		: Result(OutResult)
		, ExecutionFunction(LatentInfo.ExecutionFunction)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
	{
		Result = ESEContinue::InProgress;
		Manager->DeleteAllSlots(FOnSlotsDeleted::CreateLambda([this]() {
			Result = ESEContinue::Continue;
		}));
	}
	void UpdateOperation(FLatentResponse& Response) override
	{
		Response.FinishAndTriggerIf(
			Result != ESEContinue::InProgress, ExecutionFunction, OutputLink, CallbackTarget);
	}
#if WITH_EDITOR
	FString GetDescription() const override
	{
		return TEXT("Deleting all slots...");
	}
#endif
};


class FSELoadInfosAction : public FPendingLatentAction
{
public:
	TArray<USaveSlot*>& Slots;
	ESEContinue& Result;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	FSELoadInfosAction(USaveManager* Manager, const bool bSortByRecent, TArray<USaveSlot*>& OutSlots,
		ESEContinue& OutResult, const FLatentActionInfo& LatentInfo)
		: Slots(OutSlots)
		, Result(OutResult)
		, ExecutionFunction(LatentInfo.ExecutionFunction)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
	{
		Result = ESEContinue::InProgress;
		Manager->FindAllSlots(
			bSortByRecent, FOnSlotsLoaded::CreateLambda([this](const TArray<USaveSlot*>& Results) {
				Slots = Results;
				Result = ESEContinue::Continue;
			}));
	}
	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		Response.FinishAndTriggerIf(
			Result != ESEContinue::InProgress, ExecutionFunction, OutputLink, CallbackTarget);
	}
#if WITH_EDITOR
	virtual FString GetDescription() const override
	{
		return TEXT("Loading all slots...");
	}
#endif
};


class FSaveGameAction : public FPendingLatentAction
{
public:
	ESEContinueOrFail& Result;
	FName ExecutionFunction;
	int32 OutputLink;
	FWeakObjectPtr CallbackTarget;

	FSaveGameAction(USaveManager* Manager, FName SlotName, bool bOverrideIfNeeded, bool bScreenshot,
		const FScreenshotSize Size, ESEContinueOrFail& OutResult, const FLatentActionInfo& LatentInfo)
		: Result(OutResult)
		, ExecutionFunction(LatentInfo.ExecutionFunction)
		, OutputLink(LatentInfo.Linkage)
		, CallbackTarget(LatentInfo.CallbackTarget)
	{
		const bool bStarted = Manager->SaveSlot(SlotName, bOverrideIfNeeded, bScreenshot, Size,
			FOnGameSaved::CreateRaw(this, &FSaveGameAction::OnSaveFinished));
		Result = bStarted ? ESEContinueOrFail::InProgress : ESEContinueOrFail::Failed;
	}

	virtual void UpdateOperation(FLatentResponse& Response) override
	{
		Response.FinishAndTriggerIf(
			Result != ESEContinueOrFail::InProgress, ExecutionFunction, OutputLink, CallbackTarget);
	}
	void OnSaveFinished(USaveSlot* SavedSlot)
	{
		Result = SavedSlot ? ESEContinueOrFail::Continue : ESEContinueOrFail::Failed;
	}
#if WITH_EDITOR
	// Returns a human readable description of the latent operation's current state
	virtual FString GetDescription() const override
	{
		return TEXT("Saving Game...");
	}
#endif
};

// END Async Actions


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

	if (GetActiveSlot()->bSaveOnClose)
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

	if (SlotName.IsNone())
	{
		SELog(ActiveSlot, "Can't use an empty slot name to save.", true);
		return false;
	}

	// Saving
	SELog(ActiveSlot, "Saving to Slot " + SlotName.ToString());

	UWorld* World = GetWorld();
	check(World);

	// Launch task, always fail if it didn't finish or wasn't scheduled
	auto& Task = CreateTask<FSEDataTask_Save>()
		.Setup(SlotName, bOverrideIfNeeded, bScreenshot, Size.Width, Size.Height)
		.Bind(OnSaved)
		.Start();

	return Task.IsSucceeded() || Task.IsScheduled();
}

bool USaveManager::LoadSlot(FName SlotName, FOnGameLoaded OnLoaded)
{
	if (!CanLoadOrSave() || !IsSlotSaved(SlotName))
	{
		return false;
	}

	AssureActiveSlot();

	auto& Task = CreateTask<FSEDataTask_Load>().Setup(SlotName).Bind(OnLoaded).Start();
	return Task.IsSucceeded() || Task.IsScheduled();
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

void USaveManager::FindAllSlots(bool bSortByRecent, FOnSlotsLoaded Delegate)
{
	MTTasks.CreateTask<FLoadSlotsTask>(this, bSortByRecent, MoveTemp(Delegate))
		.OnFinished([](auto& Task) {
			Task->AfterFinish();
		})
		.StartBackgroundTask();
}

void USaveManager::FindAllSlotsSync(bool bSortByRecent, TArray<USaveSlot*>& Slots)
{
	auto Delegate = FOnSlotsLoaded::CreateLambda([&Slots](const TArray<USaveSlot*>& FoundSlots) {
		Slots = FoundSlots;
	});
	MTTasks.CreateTask<FLoadSlotsTask>(this, bSortByRecent, Delegate)
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
	ESEContinueOrFail& Result, struct FLatentActionInfo LatentInfo, bool bOverrideIfNeeded /*= true*/)
{
	if (UWorld* World = GetWorld())
	{
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
	Result = ESEContinueOrFail::Failed;
}

void USaveManager::BPLoadSlot(FName SlotName, ESEContinueOrFail& Result, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GetWorld())
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FSELoadSlotDataAction>(
				LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID,
				new FSELoadSlotDataAction(this, SlotName, Result, LatentInfo));
		}
		return;
	}
	Result = ESEContinueOrFail::Failed;
}

void USaveManager::BPFindAllSlots(const bool bSortByRecent, TArray<USaveSlot*>& SaveInfos,
	ESEContinue& Result, struct FLatentActionInfo LatentInfo)
{
	if (UWorld* World = GetWorld())
	{
		FLatentActionManager& LatentActionManager = World->GetLatentActionManager();
		if (LatentActionManager.FindExistingAction<FSELoadInfosAction>(
				LatentInfo.CallbackTarget, LatentInfo.UUID) == nullptr)
		{
			LatentActionManager.AddNewAction(LatentInfo.CallbackTarget, LatentInfo.UUID,
				new FSELoadInfosAction(this, bSortByRecent, SaveInfos, Result, LatentInfo));
		}
	}
}

void USaveManager::BPDeleteAllSlots(ESEContinue& Result, struct FLatentActionInfo LatentInfo)
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
	return FSaveFileHelpers::FileExists(SlotName.ToString());
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

void USaveManager::AssureActiveSlot(TSubclassOf<USaveSlot> ActiveSlotClass, bool bForced)
{
	if (IsInSlot() && !bForced)
		return;

	if (!ActiveSlotClass)
	{
		ActiveSlotClass = GetDefault<USaveSettings>()->ActiveSlot.Get();
		if (!ActiveSlotClass)
		{
			ActiveSlotClass = USaveSlot::StaticClass();
		}
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
		CreateTask<FSEDataTask_SaveLevel>().Setup(LevelStreaming).Start();
	}
}

void USaveManager::DeserializeStreamingLevel(ULevelStreaming* LevelStreaming)
{
	CreateTask<FSEDataTask_LoadLevel>().Setup(LevelStreaming).Start();
}

USaveSlot* USaveManager::LoadInfo(FName SlotName)
{
	if (SlotName.IsNone())
	{
		SELog(ActiveSlot, "Invalid Slot. Cant go under 0 or exceed MaxSlots", true);
		return nullptr;
	}

	auto& Task = MTTasks.CreateTask<FLoadSlotsTask>(this, SlotName).OnFinished([](auto& Task) {
		Task->AfterFinish();
	});
	Task.StartSynchronousTask();

	check(Task.IsDone());

	const auto& Infos = Task->GetLoadedSlots();
	return Infos.Num() > 0 ? Infos[0] : nullptr;
}

void USaveManager::FinishTask(FSEDataTask* Task)
{
	Tasks.RemoveAll([Task](auto& TaskPtr) { return TaskPtr.Get() == Task; });

	// Start next task
	if (Tasks.Num() > 0)
	{
		Tasks[0]->Start();
	}
}

FName USaveManager::GetFileNameFromId(const int32 SlotId) const
{
	// TODO: Expose custom names
	// if (const auto* Preset = GetActivePreset())
	//{
	//	FName Name;
	//	Preset->BPGetSlotNameFromId(SlotId, Name);
	//	return Name;
	//}
	return FName{FString::FromInt(SlotId)};
}

bool USaveManager::IsLoading() const
{
	return HasTasks() && Tasks[0]->Type == ESETaskType::Load;
}

void USaveManager::Tick(float DeltaTime)
{
	if (Tasks.Num())
	{
		FSEDataTask* Task = Tasks[0].Get();
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

void USaveManager::OnSaveBegan()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USaveManager::OnSaveBegan);

	// TODO: Needs reworking
	/*IterateSubscribedInterfaces([&Filter](auto* Object) {
		check(Object->template Implements<USaveExtensionInterface>());

		// C++ event
		if (ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object))
		{
			Interface->OnSaveBegan(Filter);
		}
		ISaveExtensionInterface::Execute_ReceiveOnSaveBegan(Object, Filter);
	});*/
}

void USaveManager::OnSaveFinished(const bool bError)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USaveManager::OnSaveFinished);

	// TODO: Needs reworking
	/*IterateSubscribedInterfaces([&Filter, bError](auto* Object) {
		check(Object->template Implements<USaveExtensionInterface>());

		// C++ event
		if (ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object))
		{
			Interface->OnSaveFinished(Filter, bError);
		}
		ISaveExtensionInterface::Execute_ReceiveOnSaveFinished(Object, Filter, bError);
	});*/

	if (!bError)
	{
		OnGameSaved.Broadcast(ActiveSlot);
	}
}

void USaveManager::OnLoadBegan()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USaveManager::OnLoadBegan);

	/*IterateSubscribedInterfaces([&Filter](auto* Object) {
		check(Object->template Implements<USaveExtensionInterface>());

		// C++ event
		if (ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object))
		{
			Interface->OnLoadBegan(Filter);
		}
		ISaveExtensionInterface::Execute_ReceiveOnLoadBegan(Object, Filter);
	});*/
}

void USaveManager::OnLoadFinished(const bool bError)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(USaveManager::OnLoadFinished);

	/*IterateSubscribedInterfaces([&Filter, bError](auto* Object) {
		check(Object->template Implements<USaveExtensionInterface>());

		// C++ event
		if (ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object))
		{
			Interface->OnLoadFinished(Filter, bError);
		}
		ISaveExtensionInterface::Execute_ReceiveOnLoadFinished(Object, Filter, bError);
	});*/

	if (!bError)
	{
		OnGameLoaded.Broadcast(ActiveSlot);
	}
}

void USaveManager::OnMapLoadStarted(const FString& MapName)
{
	SELog(ActiveSlot, "Loading Map '" + MapName + "'", FColor::Purple);
}

void USaveManager::OnMapLoadFinished(UWorld* LoadedWorld)
{
	if (IsLoading())
	{
		static_cast<FSEDataTask_Load*>(Tasks[0].Get())->OnMapLoaded();
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
