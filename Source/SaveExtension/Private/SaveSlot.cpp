// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveSlot.h"

#include <Engine/Engine.h>
#include <Engine/GameViewportClient.h>
#include <EngineUtils.h>
#include <HighResScreenshot.h>
#include <IImageWrapper.h>
#include <IImageWrapperModule.h>
#include <Misc/FileHelper.h>


USaveSlot::USaveSlot()
{
	Data = NewObject<USaveSlotData>(this, DataClass);
}

bool USaveSlot::OnSetId(int32 Id)
{
	FileName = FName{FString::FromInt(SlotId)};
}

int32 USaveSlot::OnGetId() const
{
	return FCString::Atoi(FileName.ToString());
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
		_SetThumbnailPath(FFileAdapter::GetThumbnailPath(FileName.ToString()));

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


int32 USaveSlot::GetMaxIds() const
{
	return MaxSlots <= 0 ? 16384 : MaxSlots;
}

bool USaveSlot::IsValidId(int32 CheckedId)
{
	return CheckedId >= 0 && CheckedId < GetMaxIds();
}

void USaveSlot::_SetThumbnailPath(const FString& Path)
{
	if (ThumbnailPath != Path)
	{
		ThumbnailPath = Path;
		CachedThumbnail = nullptr;
	}
}

bool USaveSlot::SetId_Implementation(int32 Id)
{
	return OnSetId(Id);
}

int32 USaveSlot::GetId_Implementation() const
{
	return OnGetId();
}