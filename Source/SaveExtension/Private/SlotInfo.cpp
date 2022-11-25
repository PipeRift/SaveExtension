// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "SlotInfo.h"

#include <EngineUtils.h>
#include <IImageWrapper.h>
#include <IImageWrapperModule.h>
#include <HighResScreenshot.h>
#include <Engine/GameViewportClient.h>
#include <Misc/FileHelper.h>
#include <Engine/Engine.h>


UTexture2D* USlotInfo::GetThumbnail() const
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
	UTexture2D* Texture{ nullptr };
	TArray<uint8> RawFileData;
	if (GEngine && FFileHelper::LoadFileToArray(RawFileData, *ThumbnailPath))
	{
		IImageWrapperModule & ImageWrapperModule = FModuleManager::LoadModuleChecked < IImageWrapperModule >(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
		{
			TArray64<uint8> UncompressedBGRA;
			if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
			{
				Texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
				void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TextureData, UncompressedBGRA.GetData(), UncompressedBGRA.Num());
				Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
				Texture->UpdateResource();
			}
		}
	}
	const_cast<USlotInfo*>(this)->CachedThumbnail = Texture;
	return Texture;
}

bool USlotInfo::CaptureThumbnail(const int32 Width /*= 640*/, const int32 Height /*= 360*/)
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
		//Set Screenshot path
		HighResScreenshotConfig.FilenameOverride = ThumbnailPath;
		//Set Screenshot Resolution
		GScreenshotResolutionX = Width;
		GScreenshotResolutionY = Height;
		Viewport->TakeHighResScreenShot();
		return true;
	}
	return false;
}

void USlotInfo::_SetThumbnailPath(const FString& Path)
{
	if (ThumbnailPath != Path)
	{
		ThumbnailPath = Path;
		CachedThumbnail = nullptr;
	}
}

