// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Automatron.h"
#include "SaveManager.h"

#include "Helpers/TestActor.h"


class FSavePresetSpec : public Automatron::FTestSpec
{
	GENERATE_SPEC(FSavePresetSpec, "SaveExtension", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter);

	USaveManager* SaveManager = nullptr;
	ATestActor* TestActor = nullptr;
	USavePreset* TestPreset = nullptr;


	FSavePresetSpec()
	{
		bReuseWorldForAllTests = false;
		bCanUsePIEWorld = false;
	}

	USavePreset* CreateTestPreset();
};

void FSavePresetSpec::Define()
{
	BeforeEach([this]()
	{
		SaveManager = USaveManager::Get(GetMainWorld());
		TestNotNull(TEXT("SaveManager"), SaveManager);
	});

	Describe("Presets", [this]()
	{
		It("SaveManager is instanced", [this]()
		{
			TestNotNull(TEXT("SaveManager"), SaveManager);
		});
	});

	Describe("Files", [this]()
	{
		xIt("Can save files synchronously", [this]() {});
		xLatentIt("Can save files asynchronously", [this](auto& Done) {});
		xIt("Can load files synchronously", [this]() {});
		xLatentIt("Can load files asynchronously", [this](auto& Done) {});
	});

	Describe("Serialization", [this]()
	{
		BeforeEach([this]()
		{
			TestPreset = CreateTestPreset();
			auto& ActorFilter = TestPreset->ActorFilter.ClassFilter;
			ActorFilter.AllowedClasses.Add(ATestActor::StaticClass());

			// We dont need Async files are tested independently
			TestPreset->MultithreadedFiles = ESaveASyncMode::OnlySync;

			SaveManager->SetActivePreset(TestPreset);

			TestActor = GetMainWorld()->SpawnActor<ATestActor>();
		});

		It("Can save an actor synchronously", [this]()
		{
			TestPreset->MultithreadedSerialization = ESaveASyncMode::OnlySync;

			TestActor->bMyBool = true;

			TestTrue("Saved", SaveManager->SaveSlot(0));
			TestTrue("MyBool is true after Save", TestActor->bMyBool);

			TestActor->bMyBool = false;

			TestTrue("Loaded", SaveManager->LoadSlot(0));
			TestTrue("MyBool is true after Load", TestActor->bMyBool);
		});

		AfterEach([this]()
		{
			if(TestActor)
			{
				TestActor->Destroy();
				TestActor = nullptr;
			}
		});
	});

	AfterEach([this]()
	{
		SaveManager = nullptr;
	});
}

USavePreset* FSavePresetSpec::CreateTestPreset()
{
	USavePreset* Preset = NewObject<USavePreset>(GetMainWorld());
	return Preset;
}
