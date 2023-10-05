// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUndoClient.h"
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "Layout/Visibility.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Views/SListView.h"


class IDetailsView;
class IDetailLayoutBuilder;
class USaveSlot;


class FSaveSlotDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

private:
	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	FSlateColor GetWarningColor() const;
	EVisibility GetWarningVisibility() const;

	TWeakObjectPtr<USaveSlot> Slot;
};
