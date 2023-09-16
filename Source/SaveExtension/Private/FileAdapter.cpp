// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "FileAdapter.h"

#include "Multithreading/SaveFileTask.h"
#include "SavePreset.h"
#include "SaveSlot.h"
#include "SaveSlotData.h"

#include <SaveGameSystem.h>
#include <Serialization/ArchiveLoadCompressedProxy.h>
#include <Serialization/ArchiveSaveCompressedProxy.h>
#include <Serialization/MemoryReader.h>
#include <Serialization/MemoryWriter.h>
#include <UObject/Package.h>
#include <UObject/UObjectGlobals.h>


static const int SE_SAVEGAME_FILE_TYPE_TAG = 0x0001;	// "sAvG"

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
	, PackageFileUEVersion(GPackageFileUEVersion)
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
	TRACE_CPUPROFILER_EVENT_SCOPE(FSaveFile::Read);

	Empty();
	FArchive& Ar = Reader.GetArchive();

	{	 // Header information
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
		Ar << PackageFileUEVersion;
		Ar << SavedEngineVersion;
		Ar.SetUEVer(PackageFileUEVersion);
		Ar.SetEngineVer(SavedEngineVersion);

		if (SaveGameFileVersion >= FSaveGameFileVersion::AddedCustomVersions)
		{
			Ar << CustomVersionFormat;
			CustomVersions.Serialize(
				Ar, static_cast<ECustomVersionSerializationFormat::Type>(CustomVersionFormat));
			Ar.SetCustomVersions(CustomVersions);
		}
	}

	Ar << InfoClassName;
	Ar << InfoBytes;

	Ar << DataClassName;
	if (bSkipData || DataClassName.IsEmpty())
	{
		return;
	}

	Ar << bIsDataCompressed;
	if (bIsDataCompressed)
	{
		TArray<uint8> CompressedDataBytes;
		Ar << CompressedDataBytes;

		TRACE_CPUPROFILER_EVENT_SCOPE(Decompression);
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
	TRACE_CPUPROFILER_EVENT_SCOPE(FSaveFile::Write);

	bIsDataCompressed = bCompressData;
	FArchive& Ar = Writer.GetArchive();

	{	 // Header information
		Ar << FileTypeTag;
		Ar << SaveGameFileVersion;
		Ar << PackageFileUEVersion;
		Ar << SavedEngineVersion;
		Ar << CustomVersionFormat;
		CustomVersions.Serialize(
			Ar, static_cast<ECustomVersionSerializationFormat::Type>(CustomVersionFormat));
	}

	Ar << InfoClassName;
	Ar << InfoBytes;

	Ar << DataClassName;
	if (!DataClassName.IsEmpty())
	{
		Ar << bIsDataCompressed;
		if (bIsDataCompressed)
		{
			TArray<uint8> CompressedDataBytes;
			{	 // Compression
				TRACE_CPUPROFILER_EVENT_SCOPE(Compression);
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

void FSaveFile::SerializeInfo(USaveSlot* SlotInfo)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSaveFile::SerializeInfo);
	check(SlotInfo);
	InfoBytes.Reset();
	InfoClassName = SlotInfo->GetClass()->GetPathName();

	FMemoryWriter BytesWriter(InfoBytes);
	FObjectAndNameAsStringProxyArchive Ar(BytesWriter, false);
	SlotInfo->Serialize(Ar);
}
void FSaveFile::SerializeData(USaveSlotData* SlotData)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSaveFile::SerializeData);
	check(SlotData);
	DataBytes.Reset();
	DataClassName = SlotData->GetClass()->GetPathName();

	FMemoryWriter BytesWriter(DataBytes);
	FObjectAndNameAsStringProxyArchive Ar(BytesWriter, false);
	SlotData->Serialize(Ar);
}

USaveSlot* FSaveFile::CreateAndDeserializeInfo(const UObject* Outer) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSaveFile::CreateAndDeserializeInfo);
	UObject* Slot = nullptr;
	FFileAdapter::DeserializeObject(Slot, InfoClassName, Outer, InfoBytes);
	return Cast<USaveSlot>(Slot);
}

void FSaveFile::CreateAndDeserializeData(USaveSlot* Slot) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FSaveFile::CreateAndDeserializeData);
	UObject* Data = nullptr;
	FFileAdapter::DeserializeObject(Data, DataClassName, Slot, DataBytes);
	Slot->AssignData(Cast<USaveSlotData>(Data));
}

bool FFileAdapter::SaveFile(FStringView SlotName, USaveSlot* Slot, const bool bUseCompression)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FFileAdapter::SaveFile);

	if (SlotName.IsEmpty())
	{
		return false;
	}

	if (!ensureMsgf(Slot, TEXT("Slot object must be valid")) ||
		!ensureMsgf(Slot->GetData(), TEXT("Slot Data object must be valid")))
	{
		return false;
	}

	FScopedFileWriter FileWriter(GetSlotPath(SlotName));
	if (FileWriter.IsValid())
	{
		FSaveFile File{};
		File.SerializeInfo(Slot);
		File.SerializeData(Slot->GetData());
		File.Write(FileWriter, bUseCompression);
		return !FileWriter.IsError();
	}
	return false;
}

USaveSlot* FFileAdapter::LoadFile(FStringView SlotName, bool bLoadData, const UObject* Outer)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FFileAdapter::LoadFile);

	FScopedFileReader Reader(GetSlotPath(SlotName));
	if (Reader.IsValid())
	{
		FSaveFile File{};
		File.Read(Reader, !bLoadData);
		USaveSlot* Slot = File.CreateAndDeserializeInfo(Outer);
		if (bLoadData)
		{
			File.CreateAndDeserializeData(Slot);
		}
		return Slot;
	}
	return nullptr;
}

bool FFileAdapter::DeleteFile(FStringView SlotName)
{
	return IFileManager::Get().Delete(*GetSlotPath(SlotName), true, false, true);
}

bool FFileAdapter::FileExists(FStringView SlotName)
{
	return IFileManager::Get().FileSize(*GetSlotPath(SlotName)) >= 0;
}

const FString& FFileAdapter::GetSaveFolder()
{
	static const FString Folder = FString::Printf(TEXT("%sSaveGames/"), *FPaths::ProjectSavedDir());
	return Folder;
}

FString FFileAdapter::GetSlotPath(FStringView SlotName)
{
	return GetSaveFolder() / FString::Printf(TEXT("%s.sav"), SlotName.GetData());
}

FString FFileAdapter::GetThumbnailPath(FStringView SlotName)
{
	return GetSaveFolder() / FString::Printf(TEXT("%s.png"), SlotName.GetData());
}

void FFileAdapter::DeserializeObject(
	UObject*& Object, FStringView ClassName, const UObject* Outer, const TArray<uint8>& Bytes)
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

	if (!Object)
	{
		if (!Outer)
		{
			Outer = GetTransientPackage();
		}

		Object = NewObject<UObject>(const_cast<UObject*>(Outer), ObjectClass);
	}
	// Can only reuse object if class matches
	else if (Object->GetClass() != ObjectClass)
	{
		return;
	}

	if (Object)
	{
		FMemoryReader Reader{Bytes};
		FObjectAndNameAsStringProxyArchive Ar(Reader, true);
		Object->Serialize(Ar);
	}
}
