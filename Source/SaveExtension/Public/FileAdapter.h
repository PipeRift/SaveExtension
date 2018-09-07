// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/SaveGame.h>
#include <EngineVersion.h>
#include <SubclassOf.h>
#include <CustomVersion.h>
#include <PlatformFeatures.h>
#include <ObjectAndNameAsStringProxyArchive.h>

#include "CustomSaveGameSystem.h"
#include "ISaveExtension.h"

class USavePreset;


/** Based on GameplayStatics to add multi-threading */
struct FSaveFileHeader
{
	FSaveFileHeader();
	FSaveFileHeader(TSubclassOf<USaveGame> ObjectType);

	void Empty();
	bool IsEmpty() const;

	void Read(FMemoryReader& MemoryReader);
	void Write(FMemoryWriter& MemoryWriter);

	int32 FileTypeTag;
	int32 SaveGameFileVersion;
	int32 PackageFileUE4Version;
	FEngineVersion SavedEngineVersion;
	int32 CustomVersionFormat;
	FCustomVersionContainer CustomVersions;
	FString SaveGameClassName;
};

/** Based on GameplayStatics to add multi-threading */
class FFileAdapter
{
public:

	static bool SaveFile(USaveGame* SaveGameObject, const FString& SlotName, const USavePreset* Preset);

	static USaveGame* LoadFile(const FString& SlotName, const USavePreset* Preset);

	static bool DeleteFile(const FString& SlotName);

	static bool DoesFileExist(const FString& SlotName);
};
