// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SSaveActorSettingsItem.h"

#include <Widgets/SBoxPanel.h>
#include <Widgets/Layout/SBorder.h>
#include <EditorStyleSet.h>


#define LOCTEXT_NAMESPACE "SSaveActorSettingsItem"

void SSaveActorSettingsItem::Construct(const FArguments& Args)
{
	TagInfo = Args._TagInfo;
	OnValueChanged = Args._OnValueChanged;

	ChildSlot
	[
		SNew(SOverlay)
		.ToolTipText(TagInfo.Tooltip)
		+ SOverlay::Slot()
		[
			SNew(SButton)
			.RenderOpacity(0.0f)
			.OnClicked(this, &SSaveActorSettingsItem::OnClick)
		]
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(this, &SSaveActorSettingsItem::GetBorderImage)
			.Padding(FMargin(6.0f, 12.0f))
			.Visibility(EVisibility::SelfHitTestInvisible)
			.HAlign(HAlign_Center)
			[
				SNew(SHorizontalBox)
				.Visibility(EVisibility::SelfHitTestInvisible)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SAssignNew(CheckBox, SCheckBox)
					.OnCheckStateChanged(this, &SSaveActorSettingsItem::OnStateChanged)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromName(TagInfo.DisplayName))
					.Visibility(EVisibility::SelfHitTestInvisible)
				]
			]
		]
	];
}


void SSaveActorSettingsItem::SetValue(bool bActive)
{
	if (CheckBox && CheckBox->GetCheckedState() != (bActive? ECheckBoxState::Checked : ECheckBoxState::Unchecked))
	{
		CheckBox->ToggleCheckedState();
	}
}

void SSaveActorSettingsItem::OnStateChanged(ECheckBoxState State)
{
	const bool bSelected = State == ECheckBoxState::Checked;
	OnValueChanged.ExecuteIfBound(TagInfo, bSelected);
}

FReply SSaveActorSettingsItem::OnClick()
{
	if (CheckBox)
	{
		CheckBox->ToggleCheckedState();
	}
	return FReply::Handled();
}

const FSlateBrush* SSaveActorSettingsItem::GetBorderImage() const
{
	if (IsHovered())
	{
		return FEditorStyle::GetBrush("DetailsView.CategoryMiddle_Hovered");
	}
	else
	{
		return FEditorStyle::GetBrush("DetailsView.CategoryMiddle");
	}
}

#undef LOCTEXT_NAMESPACE
