// Copyright 2015-2019 Piperift. All Rights Reserved.

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
		return nullptr;

	if (CachedThumbnail)
		return CachedThumbnail;

	// Load thumbnail as Texture2D
	UTexture2D* Texture{ nullptr };
	TArray<uint8> RawFileData;
	if (GEngine && FFileHelper::LoadFileToArray(RawFileData, *ThumbnailPath))
	{
		IImageWrapperModule & ImageWrapperModule = FModuleManager::LoadModuleChecked < IImageWrapperModule >(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
		if (ImageWrapper.IsValid() && ImageWrapper->SetCompressed(RawFileData.GetData(), RawFileData.Num()))
		{
			const TArray<uint8>* UncompressedBGRA = nullptr;
			if (ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, UncompressedBGRA))
			{
				Texture = UTexture2D::CreateTransient(ImageWrapper->GetWidth(), ImageWrapper->GetHeight(), PF_B8G8R8A8);
				void* TextureData = Texture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
				FMemory::Memcpy(TextureData, UncompressedBGRA->GetData(), UncompressedBGRA->Num());
				Texture->PlatformData->Mips[0].BulkData.Unlock();
				Texture->UpdateResource();
			}
		}
	}
	const_cast<USlotInfo*>(this)->CachedThumbnail = Texture;
	return Texture;
}

bool USlotInfo::CaptureThumbnail(const int32 Width /*= 640*/, const int32 Height /*= 360*/)
{
	if (!GEngine)
		return false;

	UGameViewportClient* GameViewport = GEngine->GameViewport;
	if (GameViewport)
	{
		FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
		HighResScreenshotConfig.SetHDRCapture(false);

		FViewport * Viewport = GameViewport->Viewport;
		if (Viewport)
		{
			FString Previous = ThumbnailPath;

			IFileManager* FM = &IFileManager::Get();

			if (Previous.Len() > 0 && FM->FileExists(*Previous))
			{
				FM->Delete(*Previous, false, true, true);
			}

			_SetThumbnailPath(FString::Printf(TEXT("%sSaveGames/%i_%s.%s"), *FPaths::ProjectSavedDir(), Id, *FString("SaveScreenshot"), TEXT("png")));

			//Set Screenshot path
			HighResScreenshotConfig.FilenameOverride = ThumbnailPath;
			//Set Screenshot Resolution
			GScreenshotResolutionX = Width;
			GScreenshotResolutionY = Height;
			Viewport->TakeHighResScreenShot();
			return true;
		}
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

