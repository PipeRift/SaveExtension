// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "GameInstanceSpec.h"

#include "Automatron.h"
#include "SaveManager.h"


class FSaveSpec_GameInstance : public Automatron::FTestSpec
{
	GENERATE_SPEC(FSaveSpec_GameInstance, "SaveExtension",
		EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter);

	USaveManager* SaveManager = nullptr;

	// Helper for some test delegates
	bool bFinishTick = false;

	FSaveSpec_GameInstance()
	{
		bReuseWorldForAllTests = false;
		bCanUsePIEWorld = false;

		DefaultWorldSettings.bShouldTick = true;
		DefaultWorldSettings.GameInstance = UTestGameInstance::StaticClass();
	}
};

void FSaveSpec_GameInstance::Define()
{
	BeforeEach([this]() {
		SaveManager = USaveManager::Get(GetMainWorld());
		TestNotNull(TEXT("SaveManager"), SaveManager);

		SaveManager->bTickWithGameWorld = true;

		SaveManager->AssureActiveSlot(UTestSaveSlot::StaticClass(), true);
	});

	It("GameInstance can be saved", [this]() {
		auto* GI = GetMainWorld()->GetGameInstance<UTestGameInstance>();
		GI->bMyBool = true;

		SaveManager->SaveSlot(0);

		TestTrue("Saved variable didn't change with save", GI->bMyBool);
		GI->bMyBool = false;

		SaveManager->LoadSlot(0);

		TestTrue("Saved variable loaded", GI->bMyBool);
	});

	AfterEach([this]() {
		if (SaveManager)
		{
			bFinishTick = false;
			SaveManager->DeleteAllSlots(FOnSlotsDeleted::CreateLambda([this]() {
				bFinishTick = true;
			}));

			TickWorldUntil(GetMainWorld(), true, [this](float) {
				return !bFinishTick;
			});
		}
		SaveManager = nullptr;
	});
}
