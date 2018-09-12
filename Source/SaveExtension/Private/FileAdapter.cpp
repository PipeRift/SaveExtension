// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "FileAdapter.h"

#include <UObjectGlobals.h>
#include <MemoryReader.h>
#include <MemoryWriter.h>
#include <SaveGameSystem.h>
#include <ArchiveSaveCompressedProxy.h>
#include <ArchiveLoadCompressedProxy.h>
#include "SavePreset.h"


static const int UE4_SAVEGAME_FILE_TYPE_TAG = 0x53415647;		// "sAvG"

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


/*********************
 * FSaveFileHeader
 */

FSaveFileHeader::FSaveFileHeader()
	: FileTypeTag(0)
	, SaveGameFileVersion(0)
	, PackageFileUE4Version(0)
	, CustomVersionFormat(static_cast<int32>(ECustomVersionSerializationFormat::Unknown))
{}

FSaveFileHeader::FSaveFileHeader(TSubclassOf<USaveGame> ObjectType)
	: FileTypeTag(UE4_SAVEGAME_FILE_TYPE_TAG)
	, SaveGameFileVersion(FSaveGameFileVersion::LatestVersion)
	, PackageFileUE4Version(GPackageFileUE4Version)
	, SavedEngineVersion(FEngineVersion::Current())
	, CustomVersionFormat(static_cast<int32>(ECustomVersionSerializationFormat::Latest))
	, CustomVersions(FCustomVersionContainer::GetRegistered())
	, SaveGameClassName(ObjectType->GetPathName())
{}

void FSaveFileHeader::Empty()
{
	FileTypeTag = 0;
	SaveGameFileVersion = 0;
	PackageFileUE4Version = 0;
	SavedEngineVersion.Empty();
	CustomVersionFormat = (int32)ECustomVersionSerializationFormat::Unknown;
	CustomVersions.Empty();
	SaveGameClassName.Empty();
}

bool FSaveFileHeader::IsEmpty() const
{
	return (FileTypeTag == 0);
}

void FSaveFileHeader::Read(FMemoryReader& MemoryReader)
{
	Empty();

	MemoryReader << FileTypeTag;

	if (FileTypeTag != UE4_SAVEGAME_FILE_TYPE_TAG)
	{
		// this is an old saved game, back up the file pointer to the beginning and assume version 1
		MemoryReader.Seek(0);
		SaveGameFileVersion = FSaveGameFileVersion::InitialVersion;
	}
	else
	{
		// Read version for this file format
		MemoryReader << SaveGameFileVersion;

		// Read engine and UE4 version information
		MemoryReader << PackageFileUE4Version;

		MemoryReader << SavedEngineVersion;

		MemoryReader.SetUE4Ver(PackageFileUE4Version);
		MemoryReader.SetEngineVer(SavedEngineVersion);

		if (SaveGameFileVersion >= FSaveGameFileVersion::AddedCustomVersions)
		{
			MemoryReader << CustomVersionFormat;

			CustomVersions.Serialize(MemoryReader, static_cast<ECustomVersionSerializationFormat::Type>(CustomVersionFormat));
			MemoryReader.SetCustomVersions(CustomVersions);
		}
	}

	// Get the class name
	MemoryReader << SaveGameClassName;
}

void FSaveFileHeader::Write(FMemoryWriter& MemoryWriter)
{
	// write file type tag. identifies this file type and indicates it's using proper versioning
	// since older UE4 versions did not version this data.
	MemoryWriter << FileTypeTag;

	// Write version for this file format
	MemoryWriter << SaveGameFileVersion;

	// Write out engine and UE4 version information
	MemoryWriter << PackageFileUE4Version;
	MemoryWriter << SavedEngineVersion;

	// Write out custom version data
	MemoryWriter << CustomVersionFormat;
	CustomVersions.Serialize(MemoryWriter, static_cast<ECustomVersionSerializationFormat::Type>(CustomVersionFormat));

	// Write the class name so we know what class to load to
	MemoryWriter << SaveGameClassName;
}


/*********************
* FSaveFileHeader
*/

bool FFileAdapter::SaveFile(USaveGame* SaveGameObject, const FString& SlotName, const USavePreset* Preset)
{
	check(Preset);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FileAdapter_SaveFile);

	ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
	// If we have a system and an object to save and a save name...
	if (SaveSystem && SaveGameObject && !SlotName.IsEmpty())
	{
		TArray<uint8> ObjectBytes;

		FMemoryWriter MemoryWriter(ObjectBytes, true);
		FSaveFileHeader SaveHeader(SaveGameObject->GetClass());
		SaveHeader.Write(MemoryWriter);

		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_FileAdapter_SaveFile_Serialize);
			// Serialize SaveGame Object
			FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
			SaveGameObject->Serialize(Ar);
		}

		TArray<uint8> CompressedBytes;
		//Ptr looking towards the compressed file data
		TArray<uint8>* ObjectBytesPtr = &ObjectBytes;
		if (Preset->bUseCompression)
		{
			ObjectBytesPtr = &CompressedBytes;

			// Compress SaveGame Object
			FArchiveSaveCompressedProxy Compressor = FArchiveSaveCompressedProxy(CompressedBytes, ECompressionFlags::COMPRESS_ZLIB);
			Compressor << ObjectBytes;
			Compressor.Flush();
			Compressor.FlushCache();
			Compressor.Close();
		}

		// Stuff that data into the save system with the desired file name
		return SaveSystem->SaveGame(false, *SlotName, 0, *ObjectBytesPtr);
	}
	return false;
}

USaveGame* FFileAdapter::LoadFile(const FString& SlotName, const USavePreset* Preset)
{
	check(Preset);
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FileAdapter_LoadFile);

	ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
	// If we have a save system and a valid name..
	if (SaveSystem && !SlotName.IsEmpty())
	{
		// Load raw data from slot
		TArray<uint8> ObjectBytes;

		bool bSuccess = SaveSystem->LoadGame(false, *SlotName, 0, ObjectBytes);
		if (bSuccess)
		{
			TArray<uint8> UncompressedBytes;
			//Ptr looking towards the uncompressed file data
			TArray<uint8>* ObjectBytesPtr = &ObjectBytes;

			if (Preset->bUseCompression)
			{
				ObjectBytesPtr = &UncompressedBytes;

				FArchiveLoadCompressedProxy Decompressor(ObjectBytes, ECompressionFlags::COMPRESS_ZLIB);
				if (Decompressor.GetError())
					return nullptr;

				Decompressor << UncompressedBytes;
				Decompressor.Close();
			}

			FMemoryReader MemoryReader(*ObjectBytesPtr, true);
			FSaveFileHeader SaveHeader;
			SaveHeader.Read(MemoryReader);

			// Try and find it, and failing that, load it
			UClass* SaveGameClass = FindObject<UClass>(ANY_PACKAGE, *SaveHeader.SaveGameClassName);
			if (SaveGameClass == nullptr)
				SaveGameClass = LoadObject<UClass>(nullptr, *SaveHeader.SaveGameClassName);

			// If we have a class, try and load it.
			if (SaveGameClass != nullptr)
			{
				USaveGame* SaveGameObj = NewObject<USaveGame>(GetTransientPackage(), SaveGameClass);

				{
					QUICK_SCOPE_CYCLE_COUNTER(STAT_FileAdapter_LoadFile_Deserialize);
					FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
					SaveGameObj->Serialize(Ar);
				}

				return SaveGameObj;
			}
		}
	}
	return nullptr;
}

bool FFileAdapter::DeleteFile(const FString& SlotName)
{
	if (ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem())
	{
		return SaveSystem->DeleteGame(false, *SlotName, 0);
	}
	return false;
}

bool FFileAdapter::DoesFileExist(const FString& SlotName)
{
	if (ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem())
	{
		return SaveSystem->DoesSaveGameExist(*SlotName, 0);
	}
	return false;
}
