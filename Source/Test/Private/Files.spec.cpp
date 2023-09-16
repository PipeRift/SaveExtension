// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Automatron.h"
#include "FileAdapter.h"
#include "Helpers/TestActor.h"
#include "SaveManager.h"


class FSaveSpec_Files : public Automatron::FTestSpec
{
	GENERATE_SPEC(FSaveSpec_Files, "SaveExtension.Files",
		EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter);

	USaveManager* SaveManager = nullptr;
	ATestActor* TestActor = nullptr;
	USavePreset* TestPreset = nullptr;

	// Helper for some test delegates
	bool bFinishTick = false;

	FSaveSpec_Files() : Automatron::FTestSpec()
	{
		bReuseWorldForAllTests = false;
		bCanUsePIEWorld = false;
	}
};


void FSaveSpec_Files::Define()
{
	BeforeEach([this]() {
		SaveManager = USaveManager::Get(GetMainWorld());
		TestNotNull(TEXT("SaveManager"), SaveManager);

		SaveManager->bTickWithGameWorld = true;

		// Set test preset
		TestPreset = SaveManager->SetActivePreset(USavePreset::StaticClass());
		TestPreset->MultithreadedSerialization = ESaveASyncMode::OnlySync;
	});

	It("Can save files synchronously", [this]() {
		TestPreset->MultithreadedFiles = ESaveASyncMode::OnlySync;

		TestTrue("Saved", SaveManager->SaveSlot(0));

		TestTrue("Info File exists in disk", FFileAdapter::FileExists(TEXT("0")));
	});

	It("Can save files asynchronously", [this]() {
		TestPreset->MultithreadedFiles = ESaveASyncMode::SaveAsync;
		bFinishTick = false;

		bool bSaving =
			SaveManager->SaveSlot(0, true, false, {}, FOnGameSaved::CreateLambda([this](auto* Info) {
				// Notified that files have been saved asynchronously
				TestTrue("Info File exists in disk", FFileAdapter::FileExists(TEXT("0")));
				bFinishTick = true;
			}));
		TestTrue("Started Saving", bSaving);

		// Files shouldn't exist yet
		TestFalse("Info File exists in disk", FFileAdapter::FileExists(TEXT("0")));

		TickWorldUntil(GetMainWorld(), true, [this](float) {
			return !bFinishTick;
		});
	});

	It("Can load files synchronously", [this]() {
		TestPreset->MultithreadedFiles = ESaveASyncMode::OnlySync;

		TestTrue("Saved", SaveManager->SaveSlot(0));

		USaveSlot* Slot = nullptr;
		TestNotNull("File was loaded", FFileAdapter::LoadFile(TEXT("0"), Slot, true, SaveManager));
		TestNotNull("Info is valid", Slot);
		TestNotNull("Data is valid", Slot->GetData());
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
