// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "SavePresetDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include <Widgets/Layout/SBorder.h>
#include <EditorStyleSet.h>

#include "SavePreset.h"

#define LOCTEXT_NAMESPACE "FSavePresetDetails"


/************************************************************************
 * FSavePresetDetails
 */

TSharedRef<IDetailCustomization> FSavePresetDetails::MakeInstance()
{
	return MakeShareable(new FSavePresetDetails);
}

void FSavePresetDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	if (Objects.Num() && Objects[0] != nullptr)
	{
		Settings = CastChecked<USavePreset>(Objects[0].Get());

		DetailBuilder.EditCategory(TEXT("Gameplay"));
		DetailBuilder.EditCategory(TEXT("Serialization"));

		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Asynchronous"));
		{
			Category.AddProperty(TEXT("MultithreadedSerialization"));
			Category.AddProperty(TEXT("FrameSplittedSerialization"));
			Category.AddCustomRow(LOCTEXT("AsyncWarning", "Asynchronous Warning"))
			.Visibility({ this, &FSavePresetDetails::GetWarningVisibility })
			.ValueContent()
			.MinDesiredWidth(300.f)
			.MaxDesiredWidth(400.f)
			[
				SNew(SBorder)
				.Padding(2.f)
				.BorderImage(FEditorStyle::GetBrush("ErrorReporting.EmptyBox"))
				.BorderBackgroundColor(this, &FSavePresetDetails::GetWarningColor)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("AsyncWarningText", "WARNING: Frame-splitted loading or saving is not recommended while using Level Streaming or World Composition"))
					.AutoWrapText(true)
				]
			];

			Category.AddProperty(TEXT("MaxFrameMs")).EditCondition({ this, &FSavePresetDetails::CanEditAsynchronous }, nullptr);
		}

		DetailBuilder.EditCategory(TEXT("Level Streaming"));
	}
}

FSlateColor FSavePresetDetails::GetWarningColor() const
{
	return FLinearColor{ FColor{ 234, 220, 25, 128 } };
}

EVisibility FSavePresetDetails::GetWarningVisibility() const
{
	if (Settings.IsValid())
	{
		return Settings->GetFrameSplitSerialization() == ESaveASyncMode::OnlySync ? EVisibility::Collapsed : EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

bool FSavePresetDetails::CanEditAsynchronous() const
{
	if (Settings.IsValid())
	{
		return Settings->GetFrameSplitSerialization() != ESaveASyncMode::OnlySync;
	}
	return true;
}

#undef LOCTEXT_NAMESPACE
