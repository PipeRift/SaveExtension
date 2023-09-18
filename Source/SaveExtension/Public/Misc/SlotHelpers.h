// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "SaveFileHelpers.h"

#include <CoreMinimal.h>
#include <HAL/PlatformFile.h>


struct FSlotHelpers
{
	static void FindSlotFileNames(TArray<FString>& FoundSlots);

	/** Used to find next available slot id */
	class FFindSlotVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		TArray<FString>& FoundSlots;

		FFindSlotVisitor(TArray<FString>& FoundSlots) : FoundSlots(FoundSlots) {}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override;
	};

	static FString GetWorldName(const UWorld* World)
	{
		check(World);
		const FString MapName = World->GetOutermost()->GetName();
		if (World->IsPlayInEditor())
		{
			return UWorld::RemovePIEPrefix(MapName);
		}
		return MapName;
	}
};
