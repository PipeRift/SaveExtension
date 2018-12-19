// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "Multithreading/LoadAllSlotInfosTask.h"

#include <HAL/PlatformFilemanager.h>

#include "FileAdapter.h"
#include "SavePreset.h"
#include "SaveManager.h"


void FLoadAllSlotInfosTask::DoWork()
{
	if (!Manager)
		return;

	TArray<FString> FileNames;
	GetSlotFileNames(FileNames);

	LoadedSlots.Reserve(FileNames.Num());

	for (const FString& File : FileNames)
	{
		USlotInfo* Info = LoadInfoFromFile(File);
		if (Info) {
			LoadedSlots.Add(Info);
		}
	}

	if (bSortByRecent)
	{
		LoadedSlots.Sort([](const USlotInfo& A, const USlotInfo& B) {
			return A.SaveDate > B.SaveDate;
		});
	}
}

USlotInfo* FLoadAllSlotInfosTask::LoadInfoFromFile(const FString Name) const
{
	return Cast<USlotInfo>(FFileAdapter::LoadFile(Name));
}

void FLoadAllSlotInfosTask::GetSlotFileNames(TArray<FString>& FoundFiles) const
{
	// #TODO: Make it static... somewhere else
	static const FString SaveFolder{ FString::Printf(TEXT("%sSaveGames/"), *FPaths::ProjectSavedDir()) };

	if (!SaveFolder.IsEmpty())
	{
		FFindSlotVisitor Visitor{ FoundFiles };
		Visitor.bOnlyInfos = true;
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*SaveFolder, Visitor);
	}
}

bool FLoadAllSlotInfosTask::FFindSlotVisitor::Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
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
