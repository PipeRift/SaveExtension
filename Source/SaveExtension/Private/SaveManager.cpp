// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SaveManager.h"

#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LevelStreaming.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "GameFramework/GameModeBase.h"
#include "HighResScreenshot.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "FileHelper.h"
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

bool USaveManager::SaveGameToSlot(int32 SlotId, bool bOverrideIfNeeded, bool bScreenshot, const int32 Width /*= 640*/, const int32 Height /*= 360*/)
{
	if (!CanLoadOrSave())
		return false;

	const USavePreset* Preset = GetPreset();
	if (!IsValidSlot(SlotId))
	{
		SE_LOG(Preset, "Invalid Slot. Cant go under 0 or exceed MaxSlots.", true);
		return false;
	}

	OnSaveBegan();

	//Saving
	bool bSuccess = true;

	SE_LOG(Preset, "Saving to Slot " + FString::FromInt(SlotId));

	UWorld* World = GetWorld();
	check(World);

	//TODO: Check for task errors
	CreateSaver()->Setup(SlotId, bOverrideIfNeeded)->Start();

	SE_LOG(Preset, "Finished Saving", FColor::Green);

	bSuccess = !bScreenshot || SaveThumbnail(SlotId, Width, Height);
	OnSaveFinished(!bSuccess);
	return bSuccess;
}

bool USaveManager::LoadGame(int32 Slot)
{
	if (!CanLoadOrSave())
		return false;

	if (!IsSlotSaved(Slot))
		return false;

	OnLoadBegan();

	TryInstantiateInfo();

	const USavePreset* Preset = GetPreset();
	SE_LOG(Preset, "Loading from Slot " + FString::FromInt(Slot));

	CreateLoader()->Setup(Slot)->Start();

	return true;
}

bool USaveManager::DeleteGame(int32 SlotId)
{
	if (!IsValidSlot(SlotId))
		return false;

	const FString InfoSlot = GenerateSaveSlotName(SlotId);
	const FString DataSlot = GenerateSaveDataSlotName(SlotId);
	return FFileAdapter::DeleteFile(InfoSlot) ||
		   FFileAdapter::DeleteFile(DataSlot);
}

UTexture2D* USaveManager::LoadThumbnail(int32 Slot)
{
	if (!GEngine || !IsValidSlot(Slot))
		return nullptr;

	USlotInfo* FoundInfo = LoadInfo(Slot);
	if (!FoundInfo || FoundInfo->ThumbnailPath.IsEmpty())
		return nullptr;

	FString Thumbnail = FoundInfo->ThumbnailPath;

	// Load thumbnail as Texture2D
	UTexture2D* Texture{ nullptr };
	TArray<uint8> RawFileData;
	if (FFileHelper::LoadFileToArray(RawFileData, *Thumbnail))
	{
		IImageWrapperModule & ImageWrapperModule = FModuleManager::LoadModuleChecked < IImageWrapperModule >(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		if (FFileHelper::LoadFileToArray(RawFileData, *Thumbnail))
		{
			if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
			{
				const TArray<uint8>* UncompressedBGRA = nullptr;
				if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
				{
					Texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
					void* TextureData = Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
					FMemory::Memcpy(TextureData, UncompressedBGRA->GetData(), UncompressedBGRA->Num());
					Texture->PlatformData->Mips[0].BulkData.Unlock();
					Texture->UpdateResource();
				}
			}
		}
	}
	return Texture;
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

	USlotData* LoadedData = Cast<USlotData>(FFileAdapter::LoadFile(Card));

	/**

	*/
	return LoadedData;
}

bool USaveManager::SaveThumbnail(const int32 Slot, const int32 Width /*= 640*/, const int32 Height /*= 360*/)
{
	if (!GEngine || !IsValidSlot(Slot))
	{
		return false;
	}

	TryInstantiateInfo();

	//Last thumbnail if existing
	FString Thumbnail = CurrentInfo->ThumbnailPath;

	FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
	UGameViewportClient* GameViewport = GEngine->GameViewport;

	if (!GameViewport)
	{
		return false;
	}

	HighResScreenshotConfig.SetHDRCapture(false);
	FViewport * Viewport = GameViewport->Viewport;
	if (Viewport)
	{
		FString Previous = Thumbnail;

		IFileManager* FM = &IFileManager::Get();

		if (Previous.Len() > 0 && FM->FileExists(*Previous))
		{
			FM->Delete(*Previous, false, true, true);
		}

		Thumbnail = FString::Printf(TEXT("%sSaveGames/%i_%s.%s"), *FPaths::ProjectSavedDir(), Slot, *FString("SaveScreenshot"), TEXT("png"));

		//Set Screenshot path
		HighResScreenshotConfig.FilenameOverride = Thumbnail;
		//Set Screenshot Resolution
		GScreenshotResolutionX = Width;
		GScreenshotResolutionY = Height;
		Viewport->TakeHighResScreenShot();

		CurrentInfo->ThumbnailPath = Thumbnail;
	}

	return true;
}

USlotInfo* USaveManager::LoadInfoFromFile(const FString Name) const
{
	return Cast<USlotInfo>(FFileAdapter::LoadFile(Name));
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

void USaveManager::RegistrySaveInterface(const TScriptInterface<ISaveExtensionInterface>& Interface)
{
	RegisteredInterfaces.AddUnique(Interface);
}

void USaveManager::UnregistrySaveInterface(const TScriptInterface<ISaveExtensionInterface>& Interface)
{
	RegisteredInterfaces.Remove(Interface);
}


void USaveManager::OnSaveBegan()
{
	for(auto& InterfaceScript : RegisteredInterfaces)
	{
		UObject* Object = InterfaceScript.GetObject();
		if (!Object)
			return;

		ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object);
		if (Interface) {
			Interface->Execute_OnSaveBegan(Object);
		}
		else if (Object->GetClass()->ImplementsInterface(USaveExtensionInterface::StaticClass())) {
			ISaveExtensionInterface::Execute_OnSaveBegan(Object);
		}
	}
}

void USaveManager::OnSaveFinished(const bool bError)
{
	for (auto& InterfaceScript : RegisteredInterfaces)
	{
		UObject* Object = InterfaceScript.GetObject();
		if (!Object)
			return;

		ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object);
		if (Interface) {
			Interface->Execute_OnSaveFinished(Object, bError);
		}
		else if (Object->GetClass()->ImplementsInterface(USaveExtensionInterface::StaticClass())) {
			ISaveExtensionInterface::Execute_OnSaveFinished(Object, bError);
		}
	}

	if (!bError)
	{
		OnGameSaved.Broadcast(CurrentInfo);
	}
}

void USaveManager::OnLoadBegan()
{
	for (auto& InterfaceScript : RegisteredInterfaces)
	{
		UObject* Object = InterfaceScript.GetObject();
		if (!Object)
			return;

		ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object);
		if (Interface) {
			Interface->Execute_OnLoadBegan(Object);
		}
		else if (Object->GetClass()->ImplementsInterface(USaveExtensionInterface::StaticClass())) {
			ISaveExtensionInterface::Execute_OnLoadBegan(Object);
		}
	}
}

void USaveManager::OnLoadFinished(const bool bError)
{
	for (auto& InterfaceScript : RegisteredInterfaces)
	{
		UObject* Object = InterfaceScript.GetObject();
		if (!Object)
			return;

		ISaveExtensionInterface* Interface = Cast<ISaveExtensionInterface>(Object);
		if (Interface) {
			Interface->Execute_OnLoadFinished(Object, bError);
		}
		else if (Object->GetClass()->ImplementsInterface(USaveExtensionInterface::StaticClass())) {
			ISaveExtensionInterface::Execute_OnLoadFinished(Object, bError);
		}
	}

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
	// Remove this manager form the static list
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


