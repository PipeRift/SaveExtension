// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Layout/Visibility.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Input/Reply.h"
#include "PropertyHandle.h"
#include "EditorUndoClient.h"

class IDetailsView;
class IDetailLayoutBuilder;
class USavePreset;


class FSavePresetDetails : public IDetailCustomization
{
public:

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

private:

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	FSlateColor GetWarningColor() const;
	EVisibility GetWarningVisibility() const;
	bool CanEditAsynchronous() const;


	TWeakObjectPtr<USavePreset> Settings;
};
