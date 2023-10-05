// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveSlot.h"

#include "SEFileHelpers.h"

#include <Engine/Engine.h>
#include <Engine/GameViewportClient.h>
#include <Engine/Texture2D.h>
#include <HighResScreenshot.h>
#include <ImageUtils.h>
#include <Misc/FileHelper.h>


void USaveSlot::PostInitProperties()
{
	Super::PostInitProperties();
	Data = NewObject<USaveSlotData>(this, DataClass, TEXT("SlotData"));
}

void USaveSlot::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	bool bHasThumbnail = IsValid(Thumbnail);
	Ar << bHasThumbnail;
	if (bHasThumbnail)
	{
		TArray64<uint8> ThumbnailData;
		if (Ar.IsLoading())
		{
			Ar << ThumbnailData;
			Thumbnail = FImageUtils::ImportBufferAsTexture2D(ThumbnailData);
		}
		else
		{
			uint8* MipData = static_cast<uint8*>(Thumbnail->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_ONLY));
			check( MipData != nullptr );
			int64 MipDataSize = Thumbnail->GetPlatformData()->Mips[0].BulkData.GetBulkDataSize();

			FImageView MipImage(MipData, Thumbnail->GetPlatformData()->SizeX,Thumbnail->GetPlatformData()->SizeY, 1, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
			FImageUtils::CompressImage(ThumbnailData, TEXT("PNG"), MipImage);
			Thumbnail->GetPlatformData()->Mips[0].BulkData.Unlock();
			Ar << ThumbnailData;
		}
	}
}

void USaveSlot::CaptureThumbnail(FSEOnThumbnailCaptured Callback, const int32 Width /*= 640*/, const int32 Height /*= 360*/)
{
	if (!GEngine || bCapturingThumbnail)
	{
		Callback.ExecuteIfBound(false);
		return;
	}

	auto* Viewport = GEngine->GameViewport? GEngine->GameViewport->Viewport : nullptr;
	if (!Viewport)
	{
		Callback.ExecuteIfBound(false);
		return;
	}

	FHighResScreenshotConfig& HighResScreenshotConfig = GetHighResScreenshotConfig();
	HighResScreenshotConfig.SetHDRCapture(false);
	// Set Screenshot Resolution
	GScreenshotResolutionX = Width;
	GScreenshotResolutionY = Height;
	GEngine->GameViewport->OnScreenshotCaptured().AddUObject(this, &USaveSlot::OnThumbnailCaptured);
	bCapturingThumbnail = Viewport->TakeHighResScreenShot();
	if (!bCapturingThumbnail)
	{
		GEngine->GameViewport->OnScreenshotCaptured().RemoveAll(this);
		Callback.ExecuteIfBound(false);
	}
	else
	{
		CapturedThumbnailDelegate = MoveTemp(Callback);
	}
}

bool USaveSlot::ShouldDeserializeAsync() const
{
	return MultithreadedSerialization == ESEAsyncMode::LoadAsync ||
		   MultithreadedSerialization == ESEAsyncMode::SaveAndLoadAsync;
}
bool USaveSlot::ShouldSerializeAsync() const
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
	return !ShouldDeserializeAsync() && (FrameSplittedSerialization == ESEAsyncMode::LoadAsync ||
											FrameSplittedSerialization == ESEAsyncMode::SaveAndLoadAsync);
}
bool USaveSlot::IsFrameSplitSave() const
{
	return !ShouldSerializeAsync() && (FrameSplittedSerialization == ESEAsyncMode::SaveAsync ||
										  FrameSplittedSerialization == ESEAsyncMode::SaveAndLoadAsync);
}

bool USaveSlot::ShouldLoadFileAsync() const
{
	return MultithreadedFiles == ESEAsyncMode::LoadAsync ||
		   MultithreadedFiles == ESEAsyncMode::SaveAndLoadAsync;
}
bool USaveSlot::ShouldSaveFileAsync() const
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

void USaveSlot::OnThumbnailCaptured(int32 InSizeX, int32 InSizeY, const TArray<FColor>& InImageData)
{
	if (GEngine->GameViewport)
	{
		GEngine->GameViewport->OnScreenshotCaptured().RemoveAll(this);
	}

	Thumbnail = UTexture2D::CreateTransient(InSizeX, InSizeY, PF_B8G8R8A8);
#if WITH_EDITORONLY_DATA
	Thumbnail->DeferCompression = true;
#endif
	FColor* TextureData = static_cast<FColor*>(Thumbnail->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
	for(int32 i = 0; i < InImageData.Num(); ++i, ++TextureData)
	{
		*TextureData = InImageData[i];
	}
	Thumbnail->GetPlatformData()->Mips[0].BulkData.Unlock();
	Thumbnail->UpdateResource();


	bCapturingThumbnail = false;
	CapturedThumbnailDelegate.ExecuteIfBound(true);
	CapturedThumbnailDelegate = {};
}
