// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SaveManager.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameModeBase.h"
#include "HighResScreenshot.h"
#include "Paths.h"

#include <GameDelegates.h>
#include <CoreDelegates.h>

#include "FileAdapter.h"


USaveManager::USaveManager()
	: Super()
{}

void USaveManager::Init()
{
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &USaveManager::OnMapLoadStarted);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USaveManager::OnMapLoadFinished);
	FGameDelegates::Get().GetEndPlayMapDelegate().AddUObject(this, &USaveManager::Shutdown);

	//AutoLoad
	if (GetPreset()->bAutoLoad)
		ReloadCurrentSlot();

	TryInstantiateInfo();
	UpdateLevelStreamings();

	AddToRoot();
}

void USaveManager::Shutdown()
{
	if (GetPreset()->bSaveOnExit)
		SaveCurrentSlot();

	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);

	// Destroy
	RemoveFromRoot();
	MarkPendingKill();
}

bool USaveManager::SaveSlot(int32 SlotId, bool bOverrideIfNeeded, bool bScreenshot, const int32 Width /*= 640*/, const int32 Height /*= 360*/)
{
	if (!CanLoadOrSave())
		return false;

	const USavePreset* Preset = GetPreset();
	if (!IsValidSlot(SlotId))
	{
		SE_LOG(Preset, "Invalid Slot. Cant go under 0 or exceed MaxSlots.", true);
		return false;
	}

	//Saving
	SE_LOG(Preset, "Saving to Slot " + FString::FromInt(SlotId));

	UWorld* World = GetWorld();
	check(World);

	//Launch task, always fail if it didn't finish or wasn't scheduled
	const auto* Task = CreateSaver()->Setup(SlotId, bOverrideIfNeeded, bScreenshot, Width, Height)->Start();
	return Task->IsSucceeded() || Task->IsScheduled();
}

bool USaveManager::LoadSlotFromId(int32 SlotId)
{
	if (!CanLoadOrSave())
		return false;

	if (!IsSlotSaved(SlotId))
		return false;

	TryInstantiateInfo();

	auto* Task = CreateLoader()->Setup(SlotId)->Start();
	return Task->IsSucceeded() || Task->IsScheduled();
}

bool USaveManager::DeleteSlot(int32 SlotId)
{
	if (!IsValidSlot(SlotId))
		return false;

	const FString InfoSlot = GenerateSaveSlotName(SlotId);
	const FString DataSlot = GenerateSaveDataSlotName(SlotId);
	return FFileAdapter::DeleteFile(InfoSlot) ||
		   FFileAdapter::DeleteFile(DataSlot);
}

bool USaveManager::IsSlotSaved(int32 SlotId) const
{
	if (!IsValidSlot(SlotId))
		return false;

	const FString InfoSlot = GenerateSaveSlotName(SlotId);
	const FString DataSlot = GenerateSaveDataSlotName(SlotId);
	return FFileAdapter::DoesFileExist(InfoSlot) &&
		   FFileAdapter::DoesFileExist(DataSlot);
}

void USaveManager::GetAllSlotInfos(TArray<USlotInfo*>& SaveInfos, const bool SortByRecent)
{
	const USavePreset* Preset = GetPreset();

	TArray<FString> FileNames;
	GetSlotFileNames(FileNames);

	SaveInfos.Empty();
	SaveInfos.Reserve(FileNames.Num());

	for (const FString& File : FileNames)
	{
		USlotInfo* Info = LoadInfoFromFile(File);
		if (Info) {
			SaveInfos.Add(Info);
		}
	}

	if (SortByRecent)
	{
		SaveInfos.Sort([](const USlotInfo& A, const USlotInfo& B) {
			return A.SaveDate > B.SaveDate;
		});
	}
}

FString USaveManager::EventGenerateSaveSlot_Implementation(const int32 SlotId) const
{
	if(!IsValidSlot(SlotId)) {
		return FString();
	}

	return FString::FromInt(SlotId);
}

bool USaveManager::CanLoadOrSave()
{
	const AGameModeBase* GameMode = UGameplayStatics::GetGameMode(this);
	if (!GameMode || !GameMode->HasAuthority())
		return false;

	if (!IsValid(GetWorld()))
		return false;

	return true;
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

	LevelStreamingNotifiers.Empty(Levels.Num()); // Avoid memory deallocation
	LevelStreamingNotifiers.Reserve(Levels.Num()); // Reserve extra memory
	for (auto* Level : Levels)
	{
		ULevelStreamingNotifier* Notifier = NewObject<ULevelStreamingNotifier>(this);
		Notifier->SetLevelStreaming(Level);
		Notifier->OnLevelShown().BindUFunction(this, GET_FUNCTION_NAME_CHECKED(USaveManager, DeserializeStreamingLevel));
		Notifier->OnLevelHidden().BindUFunction(this, GET_FUNCTION_NAME_CHECKED(USaveManager, SerializeStreamingLevel));
		LevelStreamingNotifiers.Add(Notifier);
	}
}

void USaveManager::SerializeStreamingLevel(ULevelStreaming* LevelStreaming)
{
	CreateLevelSaver()->Setup(LevelStreaming)->Start();
}

void USaveManager::DeserializeStreamingLevel(ULevelStreaming* LevelStreaming)
{
	CreateLevelLoader()->Setup(LevelStreaming)->Start();
}

USlotInfo* USaveManager::LoadInfo(uint32 SlotId) const
{
	if (!IsValidSlot(SlotId))
	{
		SE_LOG(GetPreset(), "Invalid Slot. Cant go under 0 or exceed MaxSlots", true);
		return false;
	}

	const FString Card = GenerateSaveSlotName(SlotId);

	return LoadInfoFromFile(Card);
}

USlotData* USaveManager::LoadData(const USlotInfo* InSaveInfo) const
{
	if (!InSaveInfo)
		return nullptr;

	const FString Card = GenerateSaveDataSlotName(InSaveInfo->Id);

	return Cast<USlotData>(FFileAdapter::LoadFile(Card, GetPreset()));
}

USlotInfo* USaveManager::LoadInfoFromFile(const FString Name) const
{
	return Cast<USlotInfo>(FFileAdapter::LoadFile(Name, GetPreset()));
}

void USaveManager::GetSlotFileNames(TArray<FString>& FoundFiles) const
{
	const FString SaveFolder{ FString::Printf(TEXT("%sSaveGames/"), *FPaths::ProjectSavedDir()) };

	if (!SaveFolder.IsEmpty())
	{
		FFindSlotVisitor Visitor { FoundFiles };
		Visitor.bOnlyInfos = true;
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*SaveFolder, Visitor);
	}
}

USlotDataTask_Saver* USaveManager::CreateSaver()
{
	return Cast<USlotDataTask_Saver>(CreateTask(USlotDataTask_Saver::StaticClass()));
}

USlotDataTask_LevelSaver* USaveManager::CreateLevelSaver()
{
	return Cast<USlotDataTask_LevelSaver>(CreateTask(USlotDataTask_LevelSaver::StaticClass()));
}

USlotDataTask_Loader* USaveManager::CreateLoader()
{
	return Cast<USlotDataTask_Loader>(CreateTask(USlotDataTask_Loader::StaticClass()));
}

USlotDataTask_LevelLoader* USaveManager::CreateLevelLoader()
{
	return Cast<USlotDataTask_LevelLoader>(CreateTask(USlotDataTask_LevelLoader::StaticClass()));
}

USlotDataTask* USaveManager::CreateTask(UClass* TaskType)
{
	USlotDataTask* Task = NewObject<USlotDataTask>(this, TaskType);
	Task->Prepare(CurrentData, GetWorld(), GetPreset());
	Tasks.Add(Task);
	return Task;
}

void USaveManager::FinishTask(USlotDataTask* Task)
{
	Tasks.Remove(Task);

	// Start next task
	if (Tasks.Num() > 0)
		Tasks[0]->Start();
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
	IterateSubscribedInterfaces([](auto* Object) {
		ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object);
		if (Interface)
		{
			Interface->Execute_OnSaveBegan(Object);
		}
		else if (Object->GetClass()->ImplementsInterface(USaveExtensionInterface::StaticClass()))
		{
			ISaveExtensionInterface::Execute_OnSaveBegan(Object);
		}
	});
}

template<bool bError>
void USaveManager::OnSaveFinished()
{
	IterateSubscribedInterfaces([](auto* Object) {
		ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object);
		if (Interface)
		{
			Interface->Execute_OnSaveFinished(Object, bError);
		}
		else if (Object->GetClass()->ImplementsInterface(USaveExtensionInterface::StaticClass()))
		{
			ISaveExtensionInterface::Execute_OnSaveFinished(Object, bError);
		}
	});

	if (!bError)
	{
		OnGameSaved.Broadcast(CurrentInfo);
	}
}

void USaveManager::OnLoadBegan()
{
	IterateSubscribedInterfaces([](auto* Object) {
		ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object);
		if (Interface)
		{
			Interface->Execute_OnLoadBegan(Object);
		}
		else if (Object->GetClass()->ImplementsInterface(USaveExtensionInterface::StaticClass()))
		{
			ISaveExtensionInterface::Execute_OnLoadBegan(Object);
		}
	});
}

template<bool bError>
void USaveManager::OnLoadFinished()
{
	IterateSubscribedInterfaces([](auto* Object) {
		ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object);
		if (Interface) {
			Interface->Execute_OnLoadFinished(Object, bError);
		}
		else if (Object->GetClass()->ImplementsInterface(USaveExtensionInterface::StaticClass())) {
			ISaveExtensionInterface::Execute_OnLoadFinished(Object, bError);
		}
	});

	if (!bError)
	{
		OnGameLoaded.Broadcast(CurrentInfo);
	}
}

void USaveManager::OnMapLoadStarted(const FString& MapName)
{
	SE_LOG(GetPreset(), "Loading Map '" + MapName + "'", FColor::Purple);
}

void USaveManager::OnMapLoadFinished(UWorld* LoadedWorld)
{
	USlotDataTask_Loader* Loader = Cast<USlotDataTask_Loader>(Tasks.Num() ? Tasks[0] : nullptr);
	if (Loader && Loader->bLoadingMap)
	{
		Loader->OnMapLoaded();
	}

	UpdateLevelStreamings();
}

UWorld* USaveManager::GetWorld() const
{
	// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
	if (HasAllFlags(RF_ClassDefaultObject) || !GetOuter())
		return nullptr;

	// Our outer should be the GameInstance
	return GetOuter()->GetWorld();
}

void USaveManager::BeginDestroy()
{
	// Remove this manager from the static list
	GlobalManagers.Remove(OwningGameInstance);
	Super::BeginDestroy();
}

bool USaveManager::FFindSlotVisitor::Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
{
	if (!bIsDirectory)
	{
		FString FullFilePath(FilenameOrDirectory);
		if (FPaths::GetExtension(FullFilePath) == TEXT("sav"))
		{
			FString CleanFilename = FPaths::GetBaseFilename(FullFilePath);
			CleanFilename.RemoveFromEnd(".sav");

			if (bOnlyInfos)
			{
				if (!CleanFilename.EndsWith("_data"))
				{
					FilesFound.Add(CleanFilename);
				}
			}
			else if (bOnlyDatas)
			{
				if (CleanFilename.EndsWith("_data"))
				{
					FilesFound.Add(CleanFilename);
				}
			}
			else
			{
				FilesFound.Add(CleanFilename);
			}
		}
	}
	return true;
}


TMap<TWeakObjectPtr<UGameInstance>, TWeakObjectPtr<USaveManager>> USaveManager::GlobalManagers {};

USaveManager* USaveManager::GetSaveManager(const UObject* ContextObject)
{
	UWorld* World = GEngine->GetWorldFromContextObject(ContextObject, EGetWorldErrorMode::LogAndReturnNull);

	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	if (GI)
	{
		TWeakObjectPtr<USaveManager>& Manager = GlobalManagers.FindOrAdd(GI);
		if (!Manager.IsValid())
		{
			Manager = NewObject<USaveManager>(GI);
			Manager->SetGameInstance(GI);
			Manager->Init();
		}
		return Manager.Get();
	}
	return nullptr;
}


