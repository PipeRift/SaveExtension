// Copyright 2015-2024 Piperift. All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include <Engine/LevelStreaming.h>

#include "LevelStreamingNotifier.generated.h"



DECLARE_DELEGATE_OneParam(FLevelNotifierLoaded, ULevelStreaming*);
DECLARE_DELEGATE_OneParam(FLevelNotifierUnloaded, ULevelStreaming*);
DECLARE_DELEGATE_OneParam(FLevelNotifierShown, ULevelStreaming*);
DECLARE_DELEGATE_OneParam(FLevelNotifierHidden, ULevelStreaming*);


/** ULevelStreamingNotifier is an adapter that expands UE4's native
 * level streaming delegates adding a ptr to the level to each delegate
 */
UCLASS(ClassGroup = SaveExtension, Transient)
class SAVEEXTENSION_API ULevelStreamingNotifier : public UObject
{
	GENERATED_BODY()

public:
	void SetLevelStreaming(ULevelStreaming* InLevelStreaming)
	{
		UnBind();

		LevelStreaming = InLevelStreaming;

		if (IsValid(InLevelStreaming))
		{
			InLevelStreaming->OnLevelLoaded.AddDynamic(this, &ULevelStreamingNotifier::OnLoaded);
			InLevelStreaming->OnLevelUnloaded.AddDynamic(this, &ULevelStreamingNotifier::OnUnloaded);
			InLevelStreaming->OnLevelShown.AddDynamic(this, &ULevelStreamingNotifier::OnShown);
			InLevelStreaming->OnLevelHidden.AddDynamic(this, &ULevelStreamingNotifier::OnHidden);
		}
	}

	FLevelNotifierLoaded& OnLevelLoaded()
	{
		return LoadedDelegate;
	}
	FLevelNotifierUnloaded& OnLevelUnloaded()
	{
		return UnloadedDelegate;
	}
	FLevelNotifierShown& OnLevelShown()
	{
		return ShownDelegate;
	}
	FLevelNotifierHidden& OnLevelHidden()
	{
		return HiddenDelegate;
	}

private:
	void UnBind()
	{
		if (LevelStreaming.IsValid())
		{
			ULevelStreaming* Level = LevelStreaming.Get();

			Level->OnLevelLoaded.RemoveDynamic(this, &ULevelStreamingNotifier::OnLoaded);
			Level->OnLevelUnloaded.RemoveDynamic(this, &ULevelStreamingNotifier::OnUnloaded);
			Level->OnLevelShown.RemoveDynamic(this, &ULevelStreamingNotifier::OnShown);
			Level->OnLevelHidden.RemoveDynamic(this, &ULevelStreamingNotifier::OnHidden);
		}
	}

	virtual void BeginDestroy() override
	{
		UnBind();
		Super::BeginDestroy();
	}


	// Associated Level Streaming
	TWeakObjectPtr<ULevelStreaming> LevelStreaming;

	FLevelNotifierLoaded LoadedDelegate;
	FLevelNotifierUnloaded UnloadedDelegate;
	FLevelNotifierShown ShownDelegate;
	FLevelNotifierHidden HiddenDelegate;


	UFUNCTION()
	void OnLoaded()
	{
		LoadedDelegate.ExecuteIfBound(LevelStreaming.Get());
	}

	UFUNCTION()
	void OnUnloaded()
	{
		UnloadedDelegate.ExecuteIfBound(LevelStreaming.Get());
	}

	UFUNCTION()
	void OnShown()
	{
		ShownDelegate.ExecuteIfBound(LevelStreaming.Get());
	}

	UFUNCTION()
	void OnHidden()
	{
		HiddenDelegate.ExecuteIfBound(LevelStreaming.Get());
	}
};
