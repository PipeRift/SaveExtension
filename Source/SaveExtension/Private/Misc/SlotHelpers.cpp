// Copyright 2015-2020 Piperift. All Rights Reserved.

#include <Misc/SlotHelpers.h>
#include <Misc/Paths.h>
#include <HAL/PlatformFilemanager.h>


void FSlotHelpers::GetSlotFileNames(TArray<FString>& FoundFiles, bool bOnlyInfos, bool bOnlyDatas)
{
	// #TODO: Make it static... somewhere else
	static const FString SaveFolder{ FString::Printf(TEXT("%sSaveGames/"), *FPaths::ProjectSavedDir()) };

	if (!SaveFolder.IsEmpty())
	{
		FFindSlotVisitor Visitor{ FoundFiles };
		Visitor.bOnlyInfos = bOnlyInfos;
		Visitor.bOnlyDatas = bOnlyDatas;
		FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*SaveFolder, Visitor);
	}
}

bool FSlotHelpers::FFindSlotVisitor::Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
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

