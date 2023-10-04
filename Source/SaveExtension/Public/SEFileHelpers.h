// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "ISaveExtension.h"

#include <Containers/StringView.h>
#include <CoreMinimal.h>
#include <GameFramework/SaveGame.h>
#include <Misc/EngineVersion.h>
#include <PlatformFeatures.h>
#include <Serialization/CustomVersion.h>
#include <Serialization/ObjectAndNameAsStringProxyArchive.h>
#include <Templates/SubclassOf.h>
#include <Tasks/Task.h>


class USaveSlot;
class USaveSlotData;
class FMemoryReader;
class FMemoryWriter;


struct FScopedFileWriter
{
private:
	FArchive* Writer = nullptr;

public:
	FScopedFileWriter(FStringView Filename, int32 Flags = 0);
	~FScopedFileWriter()
	{
		delete Writer;
	}

	FArchive& GetArchive()
	{
		return *Writer;
	}
	bool IsValid() const
	{
		return Writer != nullptr;
	}
	bool IsError() const
	{
		return Writer && (Writer->IsError() || Writer->IsCriticalError());
	}
};


struct FScopedFileReader
{
private:
	FScopedLoadingState ScopedLoadingState;
	FArchive* Reader = nullptr;

public:
	FScopedFileReader(FStringView Filename, int32 Flags = 0);
	~FScopedFileReader()
	{
		delete Reader;
	}

	FArchive& GetArchive()
	{
		return *Reader;
	}
	bool IsValid() const
	{
		return Reader != nullptr;
	}
};


/** Based on GameplayStatics to add multi-threading */
struct FSaveFile
{
	int32 FileTypeTag = 0;
	int32 SaveGameFileVersion = 0;
	FPackageFileVersion PackageFileUEVersion{};
	FEngineVersion SavedEngineVersion;
	int32 CustomVersionFormat = int32(ECustomVersionSerializationFormat::Unknown);
	FCustomVersionContainer CustomVersions;

	FString ClassName;
	TArray<uint8> Bytes;

	FString DataClassName;
	bool bIsDataCompressed = false;
	TArray<uint8> DataBytes;


	FSaveFile();

	void Empty();
	bool IsEmpty() const;

	void Read(FScopedFileReader& Reader, bool bSkipData);
	void Write(FScopedFileWriter& Writer, bool bCompressData);

	void SerializeInfo(USaveSlot* Slot);
	void SerializeData(USaveSlotData* SlotData);
};


/** Based on GameplayStatics to add multi-threading */
class SAVEEXTENSION_API FSEFileHelpers
{
public:
	static bool SaveFileSync(USaveSlot* Slot, FStringView OverrideSlotName = {}, const bool bUseCompression = true);
	static UE::Tasks::TTask<bool> SaveFile(USaveSlot* Slot, FString OverrideSlotName = {}, const bool bUseCompression = true);

	static USaveSlot* LoadFileSync(FStringView SlotName, USaveSlot* SlotHint, bool bLoadData, const USaveManager* Manager);
	static UE::Tasks::TTask<USaveSlot*> LoadFile(FString SlotName, USaveSlot* SlotHint, bool bLoadData, const USaveManager* Manager);

	static bool DeleteFile(FStringView SlotName);
	static bool FileExists(FStringView SlotName);

	static const FString& GetSaveFolder();
	static FString GetSlotPath(FStringView SlotName);

	static UObject* DeserializeObject(
		UObject* Hint, FStringView ClassName, const UObject* Outer, const TArray<uint8>& Bytes);


	static void FindAllFilesSync(TArray<FString>& FoundSlots);

	// @return the pipe used for save file operations
	static class UE::Tasks::FPipe& GetPipe();
};
