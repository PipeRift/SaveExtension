// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "GameInstance.spec.h"

#include "Automatron.h"
#include "SaveManager.h"


class FSaveSpec_GameInstance : public Automatron::FTestSpec
{
	GENERATE_SPEC(FSaveSpec_GameInstance, "SaveExtension.GameInstance",
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter);

	TObjectPtr<USaveManager> SaveManager;

	// Helper for some test delegates
	bool bFinishTick = false;

	FSaveSpec_GameInstance()
	{
		bReuseWorldForAllTests = false;
		bCanUsePIEWorld = false;

		DefaultWorldSettings.bShouldTick = true;
		DefaultWorldSettings.GameInstance = USETestGameInstance::StaticClass();
	}
};

void FSaveSpec_GameInstance::Define()
{
	BeforeEach([this]() {
		SaveManager = USaveManager::Get(GetWorld());
		TestNotNull(TEXT("SaveManager"), SaveManager.Get());

		SaveManager->bTickWithGameWorld = true;

		SaveManager->EnsureActiveSlot(USETestSaveSlot::StaticClass(), true);
	});

	It("GameInstance can be saved", [this]() {
		auto* GI = GetWorld()->GetGameInstance<USETestGameInstance>();
		GI->bMyBool = true;

		SaveManager->SaveSlot("0");

		TestTrue("Saved variable didn't change with save", GI->bMyBool);
		GI->bMyBool = false;

		SaveManager->LoadSlot("0");

		TestTrue("Saved variable loaded", GI->bMyBool);
	});

	AfterEach([this]() {
		if (SaveManager)
		{
			SaveManager->DeleteAllSlotsSync();
		}
		SaveManager = nullptr;
	});
}
