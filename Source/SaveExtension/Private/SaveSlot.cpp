// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveSlot.h"

#include "SaveFileHelpers.h"

#include <Engine/Engine.h>
#include <Engine/GameViewportClient.h>
#include <EngineUtils.h>
#include <HighResScreenshot.h>
#include <IImageWrapper.h>
#include <IImageWrapperModule.h>
#include <Misc/FileHelper.h>


void USaveSlot::PostInitProperties()
{
	Super::PostInitProperties();
	Data = NewObject<USaveSlotData>(this, DataClass, TEXT("SlotData"));
}

bool USaveSlot::OnSetIndex(int32 Index)
{
	FileName = FName{FString::FromInt(Index)};
	return true;
}

int32 USaveSlot::OnGetIndex() const
{
	return FCString::Atoi(*FileName.ToString());
}

UTexture2D* USaveSlot::GetThumbnail() const
{
	if (ThumbnailPath.IsEmpty())
	{
		return nullptr;
	}

	if (CachedThumbnail)
	{
		return CachedThumbnail;
	}

	// Load thumbnail as Texture2D
	UTexture2D* Texture{nullptr};
	TArray<uint8> RawFileData;
	if (GEngine && FFileHelper::LoadFileToArray(RawFileData, *ThumbnailPath))
	{
		IImageWrapperModule& ImageWrapperModule =
			FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
		{
			TArray64<uint8> UncompressedBGRA;
			if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
			{
				Texture = UTexture2D::CreateTransient(
					ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
				void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
				Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
				Texture->UpdateResource();
			}
		}
	}
	const_cast<USaveSlot*>(this)->CachedThumbnail = Texture;
	return Texture;
}

bool USaveSlot::CaptureThumbnail(const int32 Width /*= 640*/, const int32 Height /*= 360*/)
{
	if (!GEngine || !GEngine->GameViewport || FileName.IsNone())
	{
		return false;
	}

	if (auto* Viewport = GEngine->GameViewport->Viewport)
	{
		_SetThumbnailPath(FSaveFileHelpers::GetThumbnailPath(FileName.ToString()));

		// TODO: Removal of a thumbnail should be standarized in a function
		IFileManager& FM = IFileManager::Get();
		if (ThumbnailPath.Len() > 0 && FM.FileExists(*ThumbnailPath))
		{
			FM.Delete(*ThumbnailPath, false, true, true);
		}

		FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
		HighResScreenshotConfig.SetHDRCapture(false);
		// Set Screenshot path
		HighResScreenshotConfig.FilenameOverride = ThumbnailPath;
		// Set Screenshot Resolution
		GScreenshotResolutionX = Width;
		GScreenshotResolutionY = Height;
		Viewport->TakeHighResScreenShot();
		return true;
	}
	return false;
}


int32 USaveSlot::GetMaxIndexes() const
{
	return MaxSlots <= 0 ? 16384 : MaxSlots;
}

bool USaveSlot::IsValidIndex(int32 Index) const
{
	return Index >= 0 && Index < GetMaxIndexes();
}

void USaveSlot::_SetThumbnailPath(const FString& Path)
{
	if (ThumbnailPath != Path)
	{
		ThumbnailPath = Path;
		CachedThumbnail = nullptr;
	}
}

bool USaveSlot::SetIndex_Implementation(int32 Index)
{
	return OnSetIndex(Index);
}

int32 USaveSlot::GetIndex_Implementation() const
{
	return OnGetIndex();
}

bool USaveSlot::IsMTSerializationLoad() const
{
	return MultithreadedSerialization == ESEAsyncMode::LoadAsync ||
		   MultithreadedSerialization == ESEAsyncMode::SaveAndLoadAsync;
}
bool USaveSlot::IsMTSerializationSave() const
{
	return MultithreadedSerialization == ESEAsyncMode::SaveAsync ||
		   MultithreadedSerialization == ESEAsyncMode::SaveAndLoadAsync;
}

ESEAsyncMode USaveSlot::GetFrameSplitSerialization() const
{
	return FrameSplittedSerialization;
}
float USaveSlot::GetMaxFrameMs() const
{
	return MaxFrameMs;
}

bool USaveSlot::IsFrameSplitLoad() const
{
	return !IsMTSerializationLoad() && (FrameSplittedSerialization == ESEAsyncMode::LoadAsync ||
										   FrameSplittedSerialization == ESEAsyncMode::SaveAndLoadAsync);
}
bool USaveSlot::IsFrameSplitSave() const
{
	return !IsMTSerializationSave() && (FrameSplittedSerialization == ESEAsyncMode::SaveAsync ||
										   FrameSplittedSerialization == ESEAsyncMode::SaveAndLoadAsync);
}

bool USaveSlot::IsMTFilesLoad() const
{
	return MultithreadedFiles == ESEAsyncMode::LoadAsync ||
		   MultithreadedFiles == ESEAsyncMode::SaveAndLoadAsync;
}
bool USaveSlot::IsMTFilesSave() const
{
	return MultithreadedFiles == ESEAsyncMode::SaveAsync ||
		   MultithreadedFiles == ESEAsyncMode::SaveAndLoadAsync;
}

bool USaveSlot::IsLoadingOrSaving() const
{
	return HasAnyInternalFlags(EInternalObjectFlags::Async);
}

void USaveSlot::GetLevelFilter_Implementation(bool bIsLoading, FSELevelFilter& OutFilter) const
{
	OnGetLevelFilter(bIsLoading, OutFilter);
}

void USaveSlot::OnGetLevelFilter(bool bIsLoading, FSELevelFilter& OutFilter) const
{
	OutFilter.ActorFilter = ActorFilter;
	OutFilter.ComponentFilter = ComponentFilter;
}
