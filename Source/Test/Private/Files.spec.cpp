// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Automatron.h"
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

	FSaveSpec_Files()
	{
		bReuseWorldForAllTests = false;
		bCanUsePIEWorld = false;
	}

	USavePreset* CreateTestPreset();
};


void FSaveSpec_Files::Define()
{
	BeforeEach([this]() {
		SaveManager = USaveManager::Get(GetMainWorld());
		TestNotNull(TEXT("SaveManager"), SaveManager);

		// Set test preset
		TestPreset = CreateTestPreset();
		TestPreset->MultithreadedSerialization = ESaveASyncMode::OnlySync;
		SaveManager->SetActivePreset(TestPreset);
	});

	It("Can save files synchronously", [this]() {
		TestPreset->MultithreadedFiles = ESaveASyncMode::OnlySync;

		TestTrue("Saved", SaveManager->SaveSlot(0));

		TestTrue("Info File exists in disk", FFileAdapter::DoesFileExist("0"));
		TestTrue("Data File exists in disk", FFileAdapter::DoesFileExist("0_data"));
	});

	It("Can save files asynchronously", [this]() {
		TestPreset->MultithreadedFiles = ESaveASyncMode::SaveAsync;
		bFinishTick = false;

		bool bSaving = SaveManager->SaveSlot(0, true, false, {}, FOnGameSaved::CreateLambda([this](auto* Info) {
			// Notified that files have been saved asynchronously
			TestTrue("Info File exists in disk", FFileAdapter::DoesFileExist("0"));
			TestTrue("Data File exists in disk", FFileAdapter::DoesFileExist("0_data"));
			bFinishTick = true;
		}));
		TestTrue("Started Saving", bSaving);

		// Files shouldn't exist yet
		TestFalse("Info File exists in disk", FFileAdapter::DoesFileExist("0"));
		TestFalse("Data File exists in disk", FFileAdapter::DoesFileExist("0_data"));

		TickWorldUntil(GetMainWorld(), true, [this](float) {
			return !bFinishTick;
		});
	});

	It("Can load files synchronously", [this]() {
		TestPreset->MultithreadedFiles = ESaveASyncMode::OnlySync;

		TestTrue("Saved", SaveManager->SaveSlot(0));

		TestNotNull("File was loaded", FFileAdapter::LoadFile("0"));
		TestNotNull("File data was loaded", FFileAdapter::LoadFile("0_data"));
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

USavePreset* FSaveSpec_Files::CreateTestPreset()
{
	return NewObject<USavePreset>(GetMainWorld());
}
