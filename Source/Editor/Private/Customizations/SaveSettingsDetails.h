// Copyright 2015-2018 Piperift. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"
#include "Layout/Visibility.h"
#include "SListView.h"
#include "SEditableTextBox.h"
#include "Input/Reply.h"
#include "PropertyHandle.h"
#include "EditorUndoClient.h"

class IDetailsView;
class IDetailLayoutBuilder;
class USaveManager;


class FSaveSettingsDetails : public IDetailCustomization
{
public:

	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

private:

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	FSlateColor GetWarningColor() const;
	EVisibility GetWarningVisibility() const;


	TWeakObjectPtr<USaveManager> Settings;
};
