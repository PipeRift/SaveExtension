// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "TestHelpers.h"

#include <Engine/Engine.h>
#include <Engine/LocalPlayer.h>
#include <GameFramework/Actor.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/WorldSettings.h>
#include <EngineUtils.h>
#if WITH_EDITOR
#include <Tests/AutomationEditorPromotionCommon.h>
#include <Editor.h>
#endif


// In order to know how many tests we have defined, we need to access private members of FAutomationSpecBase!
// Thats why we replicate its layout as public for reinterpreting later.
// This is a HACK and should be removed when UE4 API updates.
struct FAutomationSpecBaseLayoutMock : public FAutomationTestBase
	, public TSharedFromThis<FAutomationSpecBase>
{
	struct FSpecIt
	{
		FString Description;
		FString Id;
		FString Filename;
		int32 LineNumber;
		TSharedRef<IAutomationLatentCommand> Command;
	};

	struct FSpecDefinitionScope
	{
		FString Description;

		TArray<TSharedRef<IAutomationLatentCommand>> BeforeEach;
		TArray<TSharedRef<FSpecIt>> It;
		TArray<TSharedRef<IAutomationLatentCommand>> AfterEach;

		TArray<TSharedRef<FSpecDefinitionScope>> Children;
	};

	struct FSpec
	{
		FString Id;
		FString Description;
		FString Filename;
		int32 LineNumber;
		TArray<TSharedRef<IAutomationLatentCommand>> Commands;
	};

	FTimespan DefaultTimeout;
	bool bEnableSkipIfError;
	TArray<FString> Description;
	TMap<FString, TSharedRef<FSpec>> IdToSpecMap;
	TSharedPtr<FSpecDefinitionScope> RootDefinitionScope;
	TArray<TSharedRef<FSpecDefinitionScope>> DefinitionScopeStack;
	bool bHasBeenDefined;
};
static_assert(sizeof(FAutomationSpecBase) == sizeof(FAutomationSpecBaseLayoutMock), "Layout mock has wrong size. Maybe FAutomationSpecBase changed?");


void FSaveSpec::PrepareTestWorld(FSaveTestOnWorldReady OnWorldReady)
{
	checkf(!IsInGameThread(), TEXT("PrepareTestWorld can only be done asynchronously. (LatentBeforeEach with ThreadPool or TaskGraph)"));

	UWorld* SelectedWorld = nullptr;
#if WITH_EDITOR
	SelectedWorld = FindGameEditorWorld();

	// If there was no PIE world, start it and try again
	if (!SelectedWorld && GIsEditor)
	{
		bWorldIsReady = false;

		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			PIEStartedHandle = FEditorDelegates::PostPIEStarted.AddLambda([this](const bool bIsSimulating)
			{
				// Notify the thread about the world being ready
				bWorldIsReady = true;
			});
			FEditorPromotionTestUtilities::StartPIE(false);
		});

		// Wait while PIE initializes
		while(!bWorldIsReady)
		{
			FPlatformProcess::Sleep(0.005f);
		}

		bInitializedPIE = true;
		SelectedWorld = FindGameEditorWorld();
	}
#endif

	if (!SelectedWorld)
	{
		SelectedWorld = GWorld;
		if (GIsEditor)
		{
			UE_LOG(LogTemp, Warning, TEXT("Test using GWorld. Not correct for PIE"));
		}
	}

	bWorldIsReady = true;
	OnWorldReady.ExecuteIfBound(SelectedWorld);
}

void FSaveSpec::ReleaseTestWorld()
{
#if WITH_EDITOR
	if (!IsInGameThread())
	{
		AsyncTask(ENamedThreads::GameThread, [this]()
		{
			ReleaseTestWorld();
		});
		return;
	}

	if (PIEStartedHandle.IsValid())
	{
		FEditorDelegates::BeginPIE.Remove(PIEStartedHandle);
	}
#endif
}

void FSaveSpec::PreDefine()
{
	LatentBeforeEach(EAsyncExecution::ThreadPool, [this](const auto & Done)
	{
		PrepareTestWorld(FSaveTestOnWorldReady::CreateLambda([this, &Done](UWorld * InWorld)
		{
			World = InWorld;
			Done.Execute();
		}));
	});
}

void FSaveSpec::PostDefine()
{
	AfterEach([this]()
	{
		ReleaseTestWorld();

		--TestRemaining;

#if WITH_EDITOR
		// If this spec initialized a PIE world, tear it down
		if (TestRemaining <= 0 && bInitializedPIE)
		{
			FEditorPromotionTestUtilities::EndPIE();
		}
#endif
	});
}

bool FSaveSpec::RunTest(const FString& InParameters)
{
	const bool bResult = Super::RunTest(InParameters);

	auto* ExposedThis = reinterpret_cast<FAutomationSpecBaseLayoutMock*>(this);
	TestRemaining = ExposedThis->IdToSpecMap.Num();

	return bResult;
}

#if WITH_EDITOR
UWorld* FSaveSpec::FindGameEditorWorld()
{
	const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
	for (const FWorldContext& Context : WorldContexts)
	{
		if (Context.World() != nullptr)
		{
			if (Context.WorldType == EWorldType::PIE /*&& Context.PIEInstance == 0*/)
			{
				return Context.World();
			}

			if (Context.WorldType == EWorldType::Game)
			{
				return Context.World();
			}
		}
	}
	return nullptr;
}
#endif
