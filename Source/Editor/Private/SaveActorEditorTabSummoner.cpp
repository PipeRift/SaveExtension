// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SaveActorEditorTabSummoner.h"

#include <Editor.h>
#include <EditorStyleSet.h>
#include <Framework/Application/SlateApplication.h>
#include <GameFramework/Actor.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <SSCSEditor.h>
#include <ScopedTransaction.h>
#include <Styling/SlateIconFinder.h>
#include <UObject/ObjectSaveContext.h>
#include <Widgets/Images/SImage.h>


#define LOCTEXT_NAMESPACE "SaveActorEditorSummoner"

const TArray<FTagInfo> SSaveActorEditorWidget::TagList{
	{TEXT("Save Tags"), TEXT("!SaveTags"), true, LOCTEXT("tags_tooltip", "Should save tags?")},
	{TEXT("Save Transform"), TEXT("!SaveTransform"), true,
		LOCTEXT("transform_tooltip", "Should save position, rotation and scale?")},
	{TEXT("Save Physics"), TEXT("!SavePhysics"), true, LOCTEXT("physics_tooltip", "Should save physics?")}};

void SSaveActorEditorWidget::Construct(const FArguments&, TWeakPtr<FBlueprintEditor> InBlueprintEditor)
{
	bRefreshingVisuals = false;

	OnBlueprintPreCompileHandle =
		GEditor->OnBlueprintPreCompile().AddSP(this, &SSaveActorEditorWidget::OnBlueprintPreCompile);
	OnObjectPreSaveHandle =
		FCoreUObjectDelegates::OnObjectPreSave.AddSP(this, &SSaveActorEditorWidget::OnObjectPreSave);

	WeakBlueprintEditor = InBlueprintEditor;
	ChildSlot.Padding(
		10)[SNew(SVerticalBox).Visibility(this, &SSaveActorEditorWidget::GetContentVisibility) +
			SVerticalBox::Slot().AutoHeight().Padding(2)[GenerateSettingsWidget().ToSharedRef()]];

	RefreshVisuals();
}

SSaveActorEditorWidget::~SSaveActorEditorWidget()
{
	GEditor->OnBlueprintPreCompile().Remove(OnBlueprintPreCompileHandle);
	FCoreUObjectDelegates::OnObjectPreSave.Remove(OnObjectPreSaveHandle);
}

void SSaveActorEditorWidget::OnObjectPreSave(UObject* InObject, FObjectPreSaveContext Context)
{
	if (InObject && InObject == GetBlueprint())
	{
		RefreshVisuals();
	}
}

void SSaveActorEditorWidget::OnBlueprintPreCompile(UBlueprint* InBlueprint)
{
	if (InBlueprint && InBlueprint == GetBlueprint())
	{
		RefreshVisuals();
	}
}

void SSaveActorEditorWidget::OnBlueprintChanged(UBlueprint* Bueprint)
{
	RefreshVisuals();
}

void SSaveActorEditorWidget::OnSettingChanged(const FTagInfo& TagInfo, bool bValue)
{
	// Don't apply changes while refreshing
	if (bRefreshingVisuals)
		return;

	AActor* Actor = GetDefaultActor();
	if (Actor)
	{
		if (bValue ^ TagInfo.bNegated)
			Actor->Tags.AddUnique(TagInfo.Tag);
		else
			Actor->Tags.Remove(TagInfo.Tag);

		FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
		RefreshVisuals();
	}
}

UBlueprint* SSaveActorEditorWidget::GetBlueprint() const
{
	TSharedPtr<FBlueprintEditor> BlueprintEditor = WeakBlueprintEditor.Pin();
	if (BlueprintEditor.IsValid())
	{
		return BlueprintEditor->GetBlueprintObj();
	}
	return nullptr;
}

AActor* SSaveActorEditorWidget::GetDefaultActor() const
{
	UBlueprint* Blueprint = GetBlueprint();
	if (Blueprint && Blueprint->GeneratedClass)
	{
		return Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject());
	}
	return nullptr;
}

TSharedPtr<SWidget> SSaveActorEditorWidget::GenerateSettingsWidget()
{
	// TODO: This can be moved into FTagInfo
	SettingItems.Add(SNew(SSaveActorSettingsItem)
						 .TagInfo(TagList[0])
						 .OnValueChanged(this, &SSaveActorEditorWidget::OnSettingChanged));
	SettingItems.Add(SNew(SSaveActorSettingsItem)
						 .TagInfo(TagList[1])
						 .OnValueChanged(this, &SSaveActorEditorWidget::OnSettingChanged));
	SettingItems.Add(SNew(SSaveActorSettingsItem)
						 .TagInfo(TagList[2])
						 .OnValueChanged(this, &SSaveActorEditorWidget::OnSettingChanged));

	AActor* Actor = GetDefaultActor();
	if (Actor)
	{
		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
			.Padding(0)[SNew(SVerticalBox) + SVerticalBox::Slot().AutoHeight()[SettingItems[0]] +
						SVerticalBox::Slot().AutoHeight()[SettingItems[1]] +
						SVerticalBox::Slot().AutoHeight().Padding(
							6)[SNew(SBorder)
								   .BorderImage(FAppStyle::GetBrush("DetailsView.CategoryMiddle"))
								   .Padding(0)[SettingItems[2]]]];
	}
	return SNullWidget::NullWidget;
}

void SSaveActorEditorWidget::RefreshVisuals()
{
	if (!GetBlueprint())
		return;

	AActor* Actor = GetDefaultActor();
	if (Actor)
	{
		bRefreshingVisuals = true;

		for (const auto& Setting : SettingItems)
		{
			bool bTagFound = Actor->Tags.Contains(Setting->TagInfo.Tag);

			Setting->SetValue(bTagFound ^ Setting->TagInfo.bNegated);
		}

		bRefreshingVisuals = false;
	}
}

const FTabId FSaveActorEditorSummoner::TabName{FName("SaveActorTab")};

FSaveActorEditorSummoner::FSaveActorEditorSummoner(TSharedPtr<FBlueprintEditor> BlueprintEditor)
	: FWorkflowTabFactory(TabName.TabType, BlueprintEditor)
	, WeakBlueprintEditor(BlueprintEditor)
{
	bIsSingleton = true;

	TabLabel = LOCTEXT("SaveActorTabName", "Save Settings");
}

TSharedRef<SWidget> FSaveActorEditorSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SSaveActorEditorWidget, WeakBlueprintEditor);
}

#undef LOCTEXT_NAMESPACE
