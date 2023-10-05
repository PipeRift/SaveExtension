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

	TSharedRef<IPropertyHandle> MultithreadedSerialization = DetailBuilder.GetProperty(FName("MultithreadedSerialization"), USaveSlot::StaticClass());
	TSharedRef<IPropertyHandle> FrameSplittedSerialization = DetailBuilder.GetProperty(FName("FrameSplittedSerialization"), USaveSlot::StaticClass());
	TSharedRef<IPropertyHandle> MaxFrameMs = DetailBuilder.GetProperty(FName("MaxFrameMs"), USaveSlot::StaticClass());

	if (Objects.Num() && Objects[0] != nullptr)
	{
		Slot = CastChecked<USaveSlot>(Objects[0].Get());

		// Sort categories
		DetailBuilder.EditCategory(TEXT("Slot"));
		DetailBuilder.EditCategory(TEXT("Automatic"));
		DetailBuilder.EditCategory(TEXT("Serialization"));
		DetailBuilder.EditCategory(TEXT("Files"));
		DetailBuilder.EditCategory(TEXT("Async"));

		DetailBuilder.AddPropertyToCategory(MultithreadedSerialization);
		DetailBuilder.AddPropertyToCategory(FrameSplittedSerialization);
		DetailBuilder.AddCustomRowToCategory(FrameSplittedSerialization, LOCTEXT("AsyncWarning", "Asynchronous Warning"))
			.Visibility({this, &FSaveSlotDetails::GetWarningVisibility})
			.ValueContent()
			.MinDesiredWidth(300.f)
			.MaxDesiredWidth(400.f)
			[
				SNew(SBorder)
				.Padding(2.f)
				.BorderImage(FAppStyle::GetBrush("ErrorReporting.EmptyBox"))
				.BorderBackgroundColor(this, &FSaveSlotDetails::GetWarningColor)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AsyncWarningText",
						"WARNING: Frame-splitted loading or saving is not recommended "
						"while using Level Streaming or World Composition"))
					.AutoWrapText(true)
				]
			];
		DetailBuilder.AddPropertyToCategory(MaxFrameMs);
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
		return Slot->GetFrameSplitSerialization() == ESEAsyncMode::SaveAndLoadSync ? EVisibility::Collapsed
																			  : EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
