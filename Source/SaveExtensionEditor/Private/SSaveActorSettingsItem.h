// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include <Widgets/SCompoundWidget.h>
#include <Widgets/Input/SCheckBox.h>

struct FTagInfo
{
	FName DisplayName;
	FName Tag;
	bool bNegated;
	FText Tooltip;

	FTagInfo() : bNegated(false) {}
	FTagInfo(FName DisplayName, FName Tag, bool bNegated = false, FText Tooltip = {})
		: DisplayName(DisplayName)
		, Tag(Tag)
		, bNegated(bNegated)
		, Tooltip(MoveTemp(Tooltip)) {}
};


DECLARE_DELEGATE_TwoParams(FOnValueChanged, const FTagInfo&, bool)

class SSaveActorSettingsItem : public SCompoundWidget
{
public:

	SLATE_BEGIN_ARGS(SSaveActorSettingsItem) : _TagInfo() {}

		SLATE_ARGUMENT(FTagInfo, TagInfo)
		SLATE_EVENT(FOnValueChanged, OnValueChanged)

	SLATE_END_ARGS();

	void Construct(const FArguments& Args);

	void SetValue(bool bActive);

private:

	void OnStateChanged(ECheckBoxState State);
	FReply OnClick();

	const FSlateBrush* GetBorderImage() const;


public:

	FTagInfo TagInfo;

private:

	FOnValueChanged OnValueChanged;

	TSharedPtr<SCheckBox> CheckBox;
};
