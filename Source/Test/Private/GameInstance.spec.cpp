// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Automatron.h"
#include "Helpers/TestGameInstance.h"
#include "SaveManager.h"


class FSaveSpec_GameInstance : public Automatron::FTestSpec
{
	GENERATE_SPEC(FSaveSpec_GameInstance, "SaveExtension",
		EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter);

	USaveManager* SaveManager = nullptr;
	USavePreset* TestPreset = nullptr;

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

		TestPreset = SaveManager->SetActivePreset(USavePreset::StaticClass());
		TestPreset->bStoreGameInstance = true;

		TestPreset->MultithreadedFiles = ESaveASyncMode::OnlySync;
		TestPreset->MultithreadedSerialization = ESaveASyncMode::OnlySync;
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
