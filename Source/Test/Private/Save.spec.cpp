// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Automatron.h"
#include "Helpers/TestActor.h"
#include "SaveManager.h"


class FSaveSpec_Preset : public Automatron::FTestSpec
{
	GENERATE_SPEC(FSaveSpec_Preset, "SaveExtension",
		EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter);

	USaveManager* SaveManager = nullptr;
	ATestActor* TestActor = nullptr;
	USavePreset* TestPreset = nullptr;

	// Helper for some test delegates
	bool bFinishTick = false;

	FSaveSpec_Preset() : Automatron::FTestSpec()
	{
		bReuseWorldForAllTests = false;
		bCanUsePIEWorld = false;

		DefaultWorldSettings.bShouldTick = true;
	}

	void TickUntilSaveTasksFinish()
	{
		TickWorldUntil(GetMainWorld(), true, [this](float) {
			return SaveManager->HasTasks();
		});
	}
};

void FSaveSpec_Preset::Define()
{
	BeforeEach([this]() {
		SaveManager = USaveManager::Get(GetMainWorld());
		TestNotNull(TEXT("SaveManager"), SaveManager);

		SaveManager->bTickWithGameWorld = true;
	});

	It("SaveManager is instanced", [this]() {
		TestNotNull(TEXT("SaveManager"), SaveManager);
	});

	Describe("Maps", [this]() {
		It("Can load into a different map", [this]() {
			TestNotImplemented();
		});
		It("Actors are recreated when loading from a different map", [this]() {
			TestNotImplemented();
		});
	});

	Describe("Serialization", [this]() {
		BeforeEach([this]() {
			TestPreset = SaveManager->SetActivePreset(USavePreset::StaticClass());
			TestPreset->ActorFilter.ClassFilter.AllowedClasses.Add(ATestActor::StaticClass());

			// We don't need Async files are tested independently
			TestPreset->MultithreadedFiles = ESaveASyncMode::OnlySync;

			TestActor = GetMainWorld()->SpawnActor<ATestActor>();
		});

		It("Can save an actor synchronously", [this]() {
			TestPreset->MultithreadedSerialization = ESaveASyncMode::OnlySync;

			TestTrue("Saved", SaveManager->SaveSlot(0));
			TestTrue("Loaded", SaveManager->LoadSlot(0));
		});

		xIt("Can save an actor asynchronously", [this]() {
			TestNotImplemented();
		});

		Describe("Properties", [this]() {
			BeforeEach([this]() {
				TestPreset->MultithreadedSerialization = ESaveASyncMode::OnlySync;
			});

			It("bool", [this]()
			{
				TestActor->bMyBool = true;
				SaveManager->SaveSlot(0);
				TestTrue("bool didn't change after save", TestActor->bMyBool);

				TestActor->bMyBool = false;
				SaveManager->LoadSlot(0);

				TickUntilSaveTasksFinish();
				TestTrue("bool was saved", TestActor->bMyBool);
			});

			It("uint8", [this]()
			{
				TestActor->MyU8 = 34;
				SaveManager->SaveSlot(0);
				TestEqual("uint8 didn't change after save", TestActor->MyU8, 34);

				TestActor->MyU8 = 212;
				SaveManager->LoadSlot(0);
				TestEqual("uint8 was saved", TestActor->MyU8, 34);
			});

			It("uint16", [this]()
			{
				TestActor->MyU16 = 34;
				SaveManager->SaveSlot(0);
				TestEqual("uint16 didn't change after save", TestActor->MyU16, 34);

				TestActor->MyU16 = 212;
				SaveManager->LoadSlot(0);
				TestEqual("uint16 was saved", TestActor->MyU16, 34);
			});

			It("uint32", [this]()
			{
				TestActor->MyU32 = 34;
				SaveManager->SaveSlot(0);
				TestEqual("uint32 didn't change after save", TestActor->MyU32, 34);

				TestActor->MyU32 = 212;
				SaveManager->LoadSlot(0);
				TestEqual("uint32 was saved", TestActor->MyU32, 34);
			});

			It("uint64", [this]()
			{
				TestActor->MyU64 = 34;
				SaveManager->SaveSlot(0);
				TestEqual("uint16 didn't change after save", TestActor->MyU64, 34);

				TestActor->MyU64 = 212;
				SaveManager->LoadSlot(0);
				TestEqual("uint16 was saved", TestActor->MyU64, 34);
			});

			It("int8", [this]()
			{
				TestActor->MyI8 = 34;
				SaveManager->SaveSlot(0);
				TestEqual("int8 didn't change after save", TestActor->MyI8, 34);

				TestActor->MyI8 = 100;
				SaveManager->LoadSlot(0);
				TestEqual("int8 was saved", TestActor->MyI8, 34);
			});

			It("int16", [this]()
			{
				TestActor->MyI16 = 34;
				SaveManager->SaveSlot(0);
				TestEqual("int16 didn't change after save", TestActor->MyI16, 34);

				TestActor->MyI16 = 212;
				SaveManager->LoadSlot(0);
				TestEqual("int16 was saved", TestActor->MyI16, 34);
			});

			It("int32", [this]()
			{
				TestActor->MyI32 = 34;
				SaveManager->SaveSlot(0);
				TestEqual("int32 didn't change after save", TestActor->MyI32, 34);

				TestActor->MyI32 = 212;
				SaveManager->LoadSlot(0);
				TestEqual("int32 was saved", TestActor->MyI32, 34);
			});

			It("int64", [this]()
			{
				TestActor->MyI64 = 34;
				SaveManager->SaveSlot(0);
				TestEqual("int64 didn't change after save", TestActor->MyI64, 34);

				TestActor->MyI64 = 212;
				SaveManager->LoadSlot(0);
				TestEqual("int64 was saved", TestActor->MyI64, 34);
			});
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
