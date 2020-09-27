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

		FString SaveFilename;
		FString SaveFolder;
		FString SaveExtension;
		FPaths::Split(FullFilePath, SaveFolder, SaveFilename, SaveExtension);
		if (SaveExtension == TEXT("sav"))
		{
			if (bOnlyInfos)
			{
				if (!SaveFilename.EndsWith("_data"))
				{
					// Find USlotInfo file if only if it has USlotData file
					if (FPaths::FileExists(SaveFolder / SaveFilename + TEXT("_data.") + SaveExtension))
					{
						FilesFound.Add(SaveFilename);
					}
				}
			}
			else if (bOnlyDatas)
			{
				if (SaveFilename.EndsWith("_data"))
				{
					FilesFound.Add(SaveFilename);
				}
			}
			else
			{
				FilesFound.Add(SaveFilename);
			}
		}
	}
	return true;
}

