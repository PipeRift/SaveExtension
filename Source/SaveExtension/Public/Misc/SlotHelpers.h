// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <HAL/PlatformFile.h>
#include "FileAdapter.h"


struct FSlotHelpers {

	static void FindSlotFileNames(TArray<FString>& FoundSlots);

	/** Used to find next available slot id */
	class FFindSlotVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		TArray<FString>& FoundSlots;

		FFindSlotVisitor(TArray<FString>& FoundSlots)
			: FoundSlots(FoundSlots)
		{}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override;
	};
};
