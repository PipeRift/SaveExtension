// Copyright 2015-2018 Piperift. All Rights Reserved.

#include "SaveSettingsDetails.h"

#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include <Widgets/Layout/SBorder.h>
#include <EditorStyleSet.h>

#include "SaveManager.h"

#define LOCTEXT_NAMESPACE "FSaveSettingsDetails"


/************************************************************************
 * FSaveSettingsDetails
 */

TSharedRef<IDetailCustomization> FSaveSettingsDetails::MakeInstance()
{
	return MakeShareable(new FSaveSettingsDetails);
}

void FSaveSettingsDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	DetailBuilder.GetObjectsBeingCustomized(Objects);

	if (Objects.Num() && Objects[0] != nullptr)
	{
		Settings = CastChecked<USaveManager>(Objects[0].Get());

		IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(TEXT("Asynchronous"));
		Category.AddCustomRow(LOCTEXT("AsyncWarning", "Asynchronous Warning"))
		.ValueContent()
		.MinDesiredWidth(300.f)
		.MaxDesiredWidth(400.f)
		[
			SNew(SBorder)
			.Padding(2.f)
			.BorderImage(FEditorStyle::GetBrush("ErrorReporting.EmptyBox"))
			.BorderBackgroundColor(this, &FSaveSettingsDetails::GetWarningColor)
			.Visibility(this, &FSaveSettingsDetails::GetWarningVisibility)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AsyncWarningText", "WARNING: Asynchronous loading or saving is not recommended while using Level Streaming or World Composition"))
				.AutoWrapText(true)
			]
		];
	}
}

FSlateColor FSaveSettingsDetails::GetWarningColor() const
{
	return FLinearColor{ FColor{ 234, 220, 25, 128 } };
}

EVisibility FSaveSettingsDetails::GetWarningVisibility() const
{
	if (Settings.IsValid())
	{
		return Settings->GetAsyncMode() == ESaveASyncMode::OnlySync ? EVisibility::Hidden : EVisibility::Visible;
	}
	return EVisibility::Collapsed;
}

#undef LOCTEXT_NAMESPACE
