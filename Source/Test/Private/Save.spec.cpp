// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Automatron.h"
#include "Helpers/TestActor.h"
#include "SaveManager.h"

class FSavePresetSpec : public Automatron::FTestSpec
{
	GENERATE_SPEC(
		FSavePresetSpec, "SaveExtension", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter);

	USaveManager* SaveManager = nullptr;
	ATestActor* TestActor = nullptr;
	USavePreset* TestPreset = nullptr;

	// Helper for some test delegates
	bool bFinishTick = false;

	FSavePresetSpec()
	{
		bReuseWorldForAllTests = false;
		bCanUsePIEWorld = false;

		DefaultWorldSettings.bShouldTick = true;
	}

	USavePreset* CreateTestPreset();
};

void FSavePresetSpec::Define()
{
	BeforeEach([this]() {
		SaveManager = USaveManager::Get(GetMainWorld());
		TestNotNull(TEXT("SaveManager"), SaveManager);
	});

	Describe("Presets", [this]() { It("SaveManager is instanced", [this]() { TestNotNull(TEXT("SaveManager"), SaveManager); }); });

	Describe("Files", [this]() {
		BeforeEach([this]() {
			TestPreset = CreateTestPreset();
			TestPreset->ActorFilter.ClassFilter.AllowedClasses.Add(ATestActor::StaticClass());
			TestPreset->MultithreadedSerialization = ESaveASyncMode::OnlySync;

			// We don't need Async files are tested independently

			SaveManager->SetActivePreset(TestPreset);
		});

		It("Can save files synchronously", [this]() {
			TestPreset->MultithreadedFiles = ESaveASyncMode::OnlySync;

			TestTrue("Saved", SaveManager->SaveSlot(0));

			TestTrue("Info File exists in disk", FFileAdapter::DoesFileExist("0"));
			TestTrue("Data File exists in disk", FFileAdapter::DoesFileExist("0_data"));
		});

		It("Can save files asynchronously", [this]()
		{
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

			TickWorldUntil(GetMainWorld(), true, [this](float)
			{
				return !bFinishTick;
			});
		});

		It("Can load files synchronously", [this]() {
			TestPreset->MultithreadedFiles = ESaveASyncMode::OnlySync;
		});

		LatentIt("Can load files asynchronously", [this](const FDoneDelegate& Done) {
			TestPreset->MultithreadedFiles = ESaveASyncMode::LoadAsync;
			Done.Execute();
		});
	});

	Describe("Serialization", [this]() {
		BeforeEach([this]() {
			TestPreset = CreateTestPreset();
			TestPreset->ActorFilter.ClassFilter.AllowedClasses.Add(ATestActor::StaticClass());

			// We dont need Async files are tested independently
			TestPreset->MultithreadedFiles = ESaveASyncMode::OnlySync;

			SaveManager->SetActivePreset(TestPreset);

			TestActor = GetMainWorld()->SpawnActor<ATestActor>();
		});

		It("Can save an actor synchronously", [this]() {
			TestPreset->MultithreadedSerialization = ESaveASyncMode::OnlySync;

			TestActor->bMyBool = true;

			TestTrue("Saved", SaveManager->SaveSlot(0));
			TestTrue("MyBool is true after Save", TestActor->bMyBool);

			TestActor->bMyBool = false;

			TestTrue("Loaded", SaveManager->LoadSlot(0));
			TestTrue("MyBool is true after Load", TestActor->bMyBool);
		});

		xIt("Can save an actor asynchronously", [this]() {
			TestPreset->MultithreadedSerialization = ESaveASyncMode::OnlySync;

			TestActor->bMyBool = true;

			TestTrue("Saved", SaveManager->SaveSlot(0));
			TestTrue("MyBool is true after Save", TestActor->bMyBool);

			TestActor->bMyBool = false;

			TestTrue("Loaded", SaveManager->LoadSlot(0));
			TestTrue("MyBool is true after Load", TestActor->bMyBool);
		});

		Describe("Properties", [this]() {

		});

		AfterEach([this]() {
			if (TestActor)
			{
				TestActor->Destroy();
				TestActor = nullptr;
			}
		});
	});

	AfterEach([this]() {
		if (SaveManager)
		{
			bFinishTick = false;
			SaveManager->DeleteAllSlots(FOnSlotsDeleted::CreateLambda([this]()
			{
					bFinishTick = true;
			}));

			TickWorldUntil(GetMainWorld(), true, [this](float) { return !bFinishTick; });
		}
		SaveManager = nullptr;
	});
}

USavePreset* FSavePresetSpec::CreateTestPreset()
{
	USavePreset* Preset = NewObject<USavePreset>(GetMainWorld());
	return Preset;
}
