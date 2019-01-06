// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <HAL/PlatformFile.h>
#include "FileAdapter.h"


struct FSlotHelpers {

	static void GetSlotFileNames(TArray<FString>& FoundFiles, bool bOnlyInfos = false, bool bOnlyDatas = false);

	/** Used to find next available slot id */
	class FFindSlotVisitor : public IPlatformFile::FDirectoryVisitor
	{
	public:
		bool bOnlyInfos = false;
		bool bOnlyDatas = false;
		TArray<FString>& FilesFound;

		FFindSlotVisitor(TArray<FString>& Files)
			: FilesFound(Files)
		{}

		virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override;
	};
};
