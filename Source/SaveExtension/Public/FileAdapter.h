// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Containers/StringView.h>
#include <GameFramework/SaveGame.h>
#include <Misc/EngineVersion.h>
#include <Templates/SubclassOf.h>
#include <Serialization/CustomVersion.h>
#include <Serialization/ObjectAndNameAsStringProxyArchive.h>
#include <PlatformFeatures.h>

#include "ISaveExtension.h"


class USavePreset;
class USlotInfo;
class USlotData;
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

	FArchive& GetArchive() { return *Writer; }
	bool IsValid() const { return Writer != nullptr; }
	bool IsError() const { return Writer && (Writer->IsError() || Writer->IsCriticalError()); }
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

	FArchive& GetArchive() { return *Reader; }
	bool IsValid() const { return Reader != nullptr; }
};


/** Based on GameplayStatics to add multi-threading */
struct FSaveFile
{
	int32 FileTypeTag = 0;
	int32 SaveGameFileVersion = 0;
	int32 PackageFileUE4Version = 0;
	FEngineVersion SavedEngineVersion;
	int32 CustomVersionFormat = int32(ECustomVersionSerializationFormat::Unknown);
	FCustomVersionContainer CustomVersions;

	FString InfoClassName;
	TArray<uint8> InfoBytes;

	FString DataClassName;
	bool bIsDataCompressed = false;
	TArray<uint8> DataBytes;


	FSaveFile();

	void Empty();
	bool IsEmpty() const;

	void Read(FScopedFileReader& Reader, bool bSkipData);
	void Write(FScopedFileWriter& Writer, bool bCompressData);

	void SerializeInfo(USlotInfo* SlotInfo);
	void SerializeData(USlotData* SlotData);
	USlotInfo* CreateAndDeserializeInfo() const;
	USlotData* CreateAndDeserializeData() const;
};


/** Based on GameplayStatics to add multi-threading */
class SAVEEXTENSION_API FFileAdapter
{
public:

	static bool SaveFile(FStringView SlotName, USlotInfo* Info, USlotData* Data, const bool bUseCompression);

	// Not safe for Multi-threading
	static bool LoadFile(FStringView SlotName, USlotInfo*& Info, USlotData*& Data, bool bLoadData);

	static bool DeleteFile(FStringView SlotName);
	static bool DoesFileExist(FStringView SlotName);

	static FString GetSavePath(FStringView FileName);

	static void DeserializeObject(UObject*& Object, FStringView ClassName, const TArray<uint8>& Bytes);
};
