// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "SaveExtensionTest.h"
#include "Automatron.h"


IMPLEMENT_MODULE(FSaveExtensionTest, SaveExtensionTest);

void FSaveExtensionTest::StartupModule()
{
	Automatron::RegisterSpecs();
}
