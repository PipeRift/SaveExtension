// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "FileAdapter.h"

#include <UObject/UObjectGlobals.h>
#include <UObject/Package.h>
#include <Serialization/MemoryReader.h>
#include <Serialization/MemoryWriter.h>
#include <Serialization/ArchiveSaveCompressedProxy.h>
#include <Serialization/ArchiveLoadCompressedProxy.h>
#include <SaveGameSystem.h>

#include "SavePreset.h"
#include "SlotInfo.h"
#include "SlotData.h"
#include "Multithreading/SaveFileTask.h"


static const int SE_SAVEGAME_FILE_TYPE_TAG = 0x0001;		// "sAvG"

struct FSaveGameFileVersion
{
	enum Type
	{
		InitialVersion = 1,
		// serializing custom versions into the savegame data to handle that type of versioning
		AddedCustomVersions = 2,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};
};

FScopedFileWriter::FScopedFileWriter(FStringView Filename, int32 Flags)
{
	if (!Filename.IsEmpty())
	{
		Writer = IFileManager::Get().CreateFileWriter(Filename.GetData(), Flags);
	}
}

FScopedFileReader::FScopedFileReader(FStringView Filename, int32 Flags)
	: ScopedLoadingState(Filename.GetData())
{
	if (!Filename.IsEmpty())
	{
		Reader = IFileManager::Get().CreateFileReader(Filename.GetData(), Flags);
		if (!Reader && !(Flags & FILEREAD_Silent))
		{
			UE_LOG(LogSaveExtension, Warning, TEXT("Failed to read file '%s' error."), Filename.GetData());
		}
	}
}

/*********************
 * FSaveFile
 */

FSaveFile::FSaveFile()
	: FileTypeTag(SE_SAVEGAME_FILE_TYPE_TAG)
	, SaveGameFileVersion(FSaveGameFileVersion::LatestVersion)
	, PackageFileUE4Version(GPackageFileUE4Version)
	, SavedEngineVersion(FEngineVersion::Current())
	, CustomVersionFormat(static_cast<int32>(ECustomVersionSerializationFormat::Latest))
	, CustomVersions(FCurrentCustomVersions::GetAll())
{}

void FSaveFile::Empty()
{
	*this = {};
}

bool FSaveFile::IsEmpty() const
{
	return FileTypeTag == 0;
}

void FSaveFile::Read(FScopedFileReader& Reader, bool bSkipData)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveFile_Read);

	Empty();
	FArchive& Ar = Reader.GetArchive();

	{ // Header information
		Ar << FileTypeTag;
		if (FileTypeTag != SE_SAVEGAME_FILE_TYPE_TAG)
		{
			// this is an old saved game, back up the file pointer to the beginning and assume version 1
			Ar.Seek(0);
			SaveGameFileVersion = FSaveGameFileVersion::InitialVersion;
			return;
		}

		// Read version for this file format
		Ar << SaveGameFileVersion;
		// Read engine and UE4 version information
		Ar << PackageFileUE4Version;
		Ar << SavedEngineVersion;
		Ar.SetUE4Ver(PackageFileUE4Version);
		Ar.SetEngineVer(SavedEngineVersion);

		if (SaveGameFileVersion >= FSaveGameFileVersion::AddedCustomVersions)
		{
			Ar << CustomVersionFormat;
			CustomVersions.Serialize(Ar, static_cast<ECustomVersionSerializationFormat::Type>(CustomVersionFormat));
			Ar.SetCustomVersions(CustomVersions);
		}
	}

	Ar << InfoClassName;
	Ar << InfoBytes;

	Ar << DataClassName;
	if(bSkipData || DataClassName.IsEmpty())
	{
		return;
	}

	Ar << bIsDataCompressed;
	if(bIsDataCompressed)
	{
		TArray<uint8> CompressedDataBytes;
		Ar << CompressedDataBytes;

		QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveFile_Decompression);
		FArchiveLoadCompressedProxy Decompressor(CompressedDataBytes, NAME_Zlib);
		if (!Decompressor.GetError())
		{
			Decompressor << DataBytes;
			Decompressor.Close();
		}
		else
		{
			UE_LOG(LogSaveExtension, Warning, TEXT("Failed to decompress data"));
		}
	}
	else
	{
		Ar << DataBytes;
	}
}

void FSaveFile::Write(FScopedFileWriter& Writer, bool bCompressData)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveFile_Write);
	bIsDataCompressed = bCompressData;
	FArchive& Ar = Writer.GetArchive();

	{ // Header information
		Ar << FileTypeTag;
		Ar << SaveGameFileVersion;
		Ar << PackageFileUE4Version;
		Ar << SavedEngineVersion;
		Ar << CustomVersionFormat;
		CustomVersions.Serialize(Ar, static_cast<ECustomVersionSerializationFormat::Type>(CustomVersionFormat));
	}

	Ar << InfoClassName;
	Ar << InfoBytes;

	Ar << DataClassName;
	if(!DataClassName.IsEmpty())
	{
		Ar << bIsDataCompressed;
		if(bIsDataCompressed)
		{
			TArray<uint8> CompressedDataBytes;
			{ // Compression
				QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveFile_Compression);
				// Compress Object data
				FArchiveSaveCompressedProxy Compressor(CompressedDataBytes, NAME_Zlib);
				Compressor << DataBytes;
				Compressor.Close();
			}
			Ar << CompressedDataBytes;
		}
		else
		{
			Ar << DataBytes;
		}
	}
	Ar.Close();
}

void FSaveFile::SerializeInfo(USlotInfo* SlotInfo)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveFile_SerializeInfo);
	check(SlotInfo);
	InfoBytes.Reset();
	InfoClassName = SlotInfo->GetClass()->GetPathName();

	FMemoryWriter BytesWriter(InfoBytes);
	FObjectAndNameAsStringProxyArchive Ar(BytesWriter, false);
	SlotInfo->Serialize(Ar);
}
void FSaveFile::SerializeData(USlotData* SlotData)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_SaveFile_SerializeData);
	check(SlotData);
	DataBytes.Reset();
	DataClassName = SlotData->GetClass()->GetPathName();

	FMemoryWriter BytesWriter(DataBytes);
	FObjectAndNameAsStringProxyArchive Ar(BytesWriter, false);
	SlotData->Serialize(Ar);
}

USlotInfo* FSaveFile::CreateAndDeserializeInfo() const
{
	UObject* Object = nullptr;
	FFileAdapter::DeserializeObject(Object, InfoClassName, InfoBytes);
	return Cast<USlotInfo>(Object);
}

USlotData* FSaveFile::CreateAndDeserializeData() const
{
	UObject* Object = nullptr;
	FFileAdapter::DeserializeObject(Object, DataClassName, DataBytes);
	return Cast<USlotData>(Object);
}

bool FFileAdapter::SaveFile(FStringView SlotName, USlotInfo* Info, USlotData* Data, const bool bUseCompression)
{
	if (SlotName.IsEmpty())
	{
		return false;
	}

	if (!ensureMsgf(Info, TEXT("Info object must be valid")) ||
		!ensureMsgf(Data, TEXT("Data object must be valid")))
	{
		return false;
	}

	QUICK_SCOPE_CYCLE_COUNTER(STAT_FileAdapter_SaveFile);

	FScopedFileWriter FileWriter(GetSavePath(SlotName));
	if(FileWriter.IsValid())
	{
		FSaveFile File{};
		File.SerializeInfo(Info);
		File.SerializeData(Data);
		File.Write(FileWriter, bUseCompression);
		return !FileWriter.IsError();
	}
	return false;
}

bool FFileAdapter::LoadFile(FStringView SlotName, USlotInfo*& Info, USlotData*& Data, bool bLoadData)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FileAdapter_LoadFile);

	FScopedFileReader Reader(GetSavePath(SlotName));
	if(Reader.IsValid())
	{
		FSaveFile File{};
		File.Read(Reader, !bLoadData);
		Info = File.CreateAndDeserializeInfo();
		Data = File.CreateAndDeserializeData();
		return true;
	}
	return false;
}

bool FFileAdapter::DeleteFile(FStringView SlotName)
{
	return IFileManager::Get().Delete(*GetSavePath(SlotName), true, false, true);
}

bool FFileAdapter::DoesFileExist(FStringView SlotName)
{
	return IFileManager::Get().FileSize(*GetSavePath(SlotName)) >= 0;
}

FString FFileAdapter::GetSavePath(FStringView FileName)
{
	return FString::Printf(TEXT("%sSaveGames/%s.sav"), *FPaths::ProjectSavedDir(), FileName.GetData());
}

void FFileAdapter::DeserializeObject(UObject*& Object, FStringView ClassName, const TArray<uint8>& Bytes)
{
	if (ClassName.IsEmpty() || Bytes.Num() <= 0)
	{
		return;
	}

	UClass* ObjectClass = FindObject<UClass>(ANY_PACKAGE, ClassName.GetData());
	if (!ObjectClass)
	{
		ObjectClass = LoadObject<UClass>(nullptr, ClassName.GetData());
	}
	if (!ObjectClass)
	{
		return;
	}

	if(!Object)
	{
		check(IsInGameThread());
		Object = NewObject<UObject>(GetTransientPackage(), ObjectClass);
	}
	// Can only reuse object if class matches
	else if(Object->GetClass() != ObjectClass)
	{
		return;
	}

	if(Object)
	{
		FMemoryReader Reader{ Bytes };
		FObjectAndNameAsStringProxyArchive Ar(Reader, true);
		Object->Serialize(Ar);
	}
}
