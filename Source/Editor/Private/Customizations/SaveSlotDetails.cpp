// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveSlotDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"

#include <EditorStyleSet.h>
#include <SaveSlot.h>
#include <Widgets/Layout/SBorder.h>


#define LOCTEXT_NAMESPACE "FSaveSlotDetails"


/************************************************************************
 * FSaveSlotDetails
 */

TSharedRef<IDetailCustomization> FSaveSlotDetails::MakeInstance()
{
	return MakeShareable(new FSaveSlotDetails);
}

void FSaveSlotDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	if (Objects.Num() && Objects[0] != nullptr)
	{
		Slot = CastChecked<USaveSlot>(Objects[0].Get());

		DetailBuilder.EditCategory(TEXT("Gameplay"));
		DetailBuilder.EditCategory(TEXT("Serialization"));

		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Asynchronous"));
		{
			Category.AddProperty(TEXT("MultithreadedSerialization"));
			Category.AddProperty(TEXT("FrameSplittedSerialization"));
			Category.AddCustomRow(LOCTEXT("AsyncWarning", "Asynchronous Warning"))
				.Visibility({this, &FSaveSlotDetails::GetWarningVisibility})
				.ValueContent()
				.MinDesiredWidth(300.f)
				.MaxDesiredWidth(
					400.f)[SNew(SBorder)
							   .Padding(2.f)
							   .BorderImage(FAppStyle::GetBrush("ErrorReporting.EmptyBox"))
							   .BorderBackgroundColor(this, &FSaveSlotDetails::GetWarningColor)
								   [SNew(STextBlock)
										   .Text(LOCTEXT("AsyncWarningText",
											   "WARNING: Frame-splitted loading or saving is not recommended "
											   "while using Level Streaming or World Composition"))
										   .AutoWrapText(true)]];

			Category.AddProperty(TEXT("MaxFrameMs"))
				.EditCondition({this, &FSaveSlotDetails::CanEditAsynchronous}, nullptr);
		}

		DetailBuilder.EditCategory(TEXT("Level Streaming"));
	}
}

FSlateColor FSaveSlotDetails::GetWarningColor() const
{
	return FLinearColor{FColor{234, 220, 25, 128}};
}

EVisibility FSaveSlotDetails::GetWarningVisibility() const
{
	if (Slot.IsValid())
	{
		return Slot->GetFrameSplitSerialization() == ESaveASyncMode::OnlySync ? EVisibility::Collapsed
																			  : EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

bool FSaveSlotDetails::CanEditAsynchronous() const
{
	if (Slot.IsValid())
	{
		return Slot->GetFrameSplitSerialization() != ESaveASyncMode::OnlySync;
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
