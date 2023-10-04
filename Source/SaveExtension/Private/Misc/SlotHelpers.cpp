// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Misc/SlotHelpers.h"

#include <HAL/PlatformFilemanager.h>
#include <Misc/Paths.h>


void FSlotHelpers::FindSlotFileNames(TArray<FString>& FoundSlots)
{
	FFindSlotVisitor Visitor{FoundSlots};
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(
		*FSEFileHelpers::GetSaveFolder(), Visitor);
}

bool FSlotHelpers::FFindSlotVisitor::Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory)
{
	if (bIsDirectory)
	{
		return true;
	}

	const FString FullFilePath(FilenameOrDirectory);

	FString Folder;
	FString Filename;
	FString Extension;
	FPaths::Split(FullFilePath, Folder, Filename, Extension);
	if (Extension == TEXT("sav"))
	{
		FoundSlots.Add(Filename);
	}
	return true;
}
