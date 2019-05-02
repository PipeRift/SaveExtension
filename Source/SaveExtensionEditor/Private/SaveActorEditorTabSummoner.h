// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WorkflowOrientedApp/WorkflowUObjectDocuments.h"

#include "SSaveActorSettingsItem.h"


class FBlueprintEditor;

class SSaveActorEditorWidget
	: public SCompoundWidget
{
	static const TArray<FTagInfo> TagList;

public:

	SLATE_BEGIN_ARGS(SSaveActorEditorWidget){}
	SLATE_END_ARGS();

	void Construct(const FArguments&, TWeakPtr<FBlueprintEditor> InBlueprintEditor);

	~SSaveActorEditorWidget();


	void OnObjectPreSave(UObject* InObject);

	void OnBlueprintPreCompile(UBlueprint* InBlueprint);

private:

	void OnBlueprintChanged(UBlueprint* Bueprint);
	void OnSettingChanged(const FTagInfo& TagInfo, bool bValue);

	UBlueprint* GetBlueprint() const;
	AActor* GetDefaultActor() const;

	TSharedPtr<SWidget> GenerateSettingsWidget();
	void RefreshVisuals();

	EVisibility GetContentVisibility() const {
		return GetDefaultActor() ? EVisibility::Visible : EVisibility::Collapsed;
	}

	bool IsSaveEnabled() const {
		if (AActor* actor = GetDefaultActor()) {
			return !actor->ActorHasTag("!Save");
		}
		return false;
	}

	bool IsTransformEnabled() const {
		if (AActor* actor = GetDefaultActor()) {
			return !actor->ActorHasTag("!SaveTransform");
		}
		return false;
	}


	TWeakPtr<FBlueprintEditor> WeakBlueprintEditor;
	TArray<TSharedRef<SSaveActorSettingsItem>> SettingItems;
	bool bRefreshingVisuals;

	FDelegateHandle OnBlueprintPreCompileHandle;
	FDelegateHandle OnObjectSavedHandle;
};


struct FSaveActorEditorSummoner
	: public FWorkflowTabFactory
{
	static const FTabId TabName;


	FSaveActorEditorSummoner(TSharedPtr<FBlueprintEditor> BlueprintEditor);

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

protected:

	TWeakPtr<FBlueprintEditor> WeakBlueprintEditor;
};
