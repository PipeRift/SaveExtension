// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "TestHelpers.h"
#include "SaveManager.h"

namespace
{
	constexpr uint32 TestFlags = EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

#define BASE_SPEC FSaveSpec

BEGIN_TESTSPEC(FSavePresetSpec, "SaveExtension.Presets", TestFlags)
	UWorld* World = nullptr;
	USaveManager* SaveManager = nullptr;
END_TESTSPEC(FSavePresetSpec)

void FSavePresetSpec::Define()
{
	BeforeEach([this]() {
		World = GetTestWorld();
		SaveManager = USaveManager::GetSaveManager(World);
	});

	It("SaveManager is instanced", [this]() {
		TestNotNull(TEXT("SaveManager"), SaveManager);
	});

	AfterEach([this]() {});
}


TESTSPEC(FSaveActorSpec, "SaveExtension.Actors", TestFlags)
void FSaveActorSpec::Define()
{
}
