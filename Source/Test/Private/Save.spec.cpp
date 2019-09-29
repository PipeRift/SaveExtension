// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "TestHelpers.h"

namespace
{
	constexpr uint32 TestFlags = EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

#define BASE_SPEC FSaveSpec

BEGIN_TESTSPEC(FSavePresetSpec, "SaveExtension.Presets", TestFlags)
	UWorld* World;
END_TESTSPEC(FSavePresetSpec)

void FSavePresetSpec::Define()
{
	BeforeEach([this]() {
		World = CreateTestWorld();
	});

	It("Can change preset", [this]() {});

	It("This Test Succeeds", [this]() {
		TestTrue("Value", true);
	});

	It("This Test Fails", [this]() {
		TestTrue("Value", false);
	});

	AfterEach([this]() {
		DestroyTestWorld(World);
	});
}


TESTSPEC(FSaveActorSpec, "SaveExtension.Actors", TestFlags)
void FSaveActorSpec::Define()
{
}
