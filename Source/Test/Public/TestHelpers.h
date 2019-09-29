// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include <Misc/AutomationTest.h>
#include <Engine/World.h>


class FSaveSpec : public FAutomationSpecBase
{
private:

	/** Active unit test worlds */
	static TArray<UWorld*> Worlds;

	/** Unit testing worlds, pending cleanup */
	static TArray<UWorld*> PendingWorldsToCleanup;


public:

	FSaveSpec(const FString& InName, const bool bInComplexTask)
		: FAutomationSpecBase(InName, bInComplexTask)
	{}

protected:

	void TestNotImplemented()
	{
		AddWarning("Test not implemented.");
	}


	UWorld* GetPrimaryWorld() const;
	UWorld* CreateTestWorld();

	void DestroyTestWorld(UWorld* CleanupWorld);

	static void CleanupUnitTestWorlds();

	bool IsUnitTestWorld(UWorld* InWorld) const
	{
		return Worlds.Contains(InWorld);
	}
};


#define BASE_SPEC FSaveSpec

#define BEGIN_TESTSPEC_PRIVATE( TClass, PrettyName, TFlags, FileName, LineNumber ) \
	class TClass : public BASE_SPEC \
	{ \
	using Super = BASE_SPEC; \
	public: \
		TClass( const FString& InName ) \
		: Super( InName, false ) { \
			static_assert((TFlags)&EAutomationTestFlags::ApplicationContextMask, "AutomationTest has no application flag.  It shouldn't run.  See AutomationTest.h."); \
			static_assert(	(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::SmokeFilter) || \
							(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::EngineFilter) || \
							(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::ProductFilter) || \
							(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::PerfFilter) || \
							(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::StressFilter) || \
							(((TFlags)&EAutomationTestFlags::FilterMask) == EAutomationTestFlags::NegativeFilter), \
							"All AutomationTests must have exactly 1 filter type specified.  See AutomationTest.h."); \
		} \
		virtual uint32 GetTestFlags() const override { return TFlags; } \
		using Super::GetTestSourceFileName; \
		virtual FString GetTestSourceFileName() const override { return FileName; } \
		using Super::GetTestSourceFileLine; \
		virtual int32 GetTestSourceFileLine() const override { return LineNumber; } \
	protected: \
		virtual FString GetBeautifiedTestName() const override { return PrettyName; } \
		virtual void Define() override;


#if WITH_AUTOMATION_WORKER
	#define TESTSPEC( TClass, PrettyName, TFlags ) \
		BEGIN_TESTSPEC_PRIVATE(TClass, PrettyName, TFlags, __FILE__, __LINE__) \
		};\
		namespace\
		{\
			TClass TClass##AutomationSpecInstance( TEXT(#TClass) );\
		}

	#define BEGIN_TESTSPEC( TClass, PrettyName, TFlags ) \
		BEGIN_TESTSPEC_PRIVATE(TClass, PrettyName, TFlags, __FILE__, __LINE__)

	#define END_TESTSPEC( TClass ) \
		};\
		namespace\
		{\
			TClass TClass##AutomationSpecInstance( TEXT(#TClass) );\
		}
#endif

