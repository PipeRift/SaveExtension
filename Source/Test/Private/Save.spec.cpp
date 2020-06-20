// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "TestHelpers.h"
#include "SaveManager.h"

namespace
{
	constexpr uint32 TestFlags = EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

#define BASE_SPEC FSaveSpec

BEGIN_TESTSPEC(FSavePresetSpec, "SaveExtension.Presets", TestFlags)
	USaveManager* SaveManager = nullptr;
END_TESTSPEC(FSavePresetSpec)

void FSavePresetSpec::Define()
{
	PreDefine();

	BeforeEach([this]() {
		SaveManager = USaveManager::GetSaveManager(GetWorld());
	});

	It("SaveManager is instanced", [this]() {
		TestNotNull(TEXT("SaveManager"), SaveManager);
	});

	PostDefine();
}


BEGIN_TESTSPEC(FSaveActorSpec, "SaveExtension.Actors", TestFlags)
	USaveManager* SaveManager = nullptr;
END_TESTSPEC(FSaveActorSpec)

void FSaveActorSpec::Define()
{
	PreDefine();

	BeforeEach([this]() {
		SaveManager = USaveManager::GetSaveManager(GetWorld());
		TestNotNull(TEXT("SaveManager"), SaveManager);
	});

	xIt("Can save an actor", [this]() {
		TestNotImplemented();
	});

	PostDefine();
}
