// AUTOMATRON
// Version: 1.2
// Repository: https://github.com/splash-damage/automatron
// Can be used as a header-only library or implemented as a module

// BSD 3-Clause License
//
// Copyright (c) 2020, Splash Damage
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this
//   list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include <CoreMinimal.h>
#include <Engine/Engine.h>
#include <Engine/GameInstance.h>
#include <EngineUtils.h>
#include <GameFramework/GameModeBase.h>
#include <GameMapsSettings.h>
#include <Misc/AutomationTest.h>
#include <Tests/AutomationCommon.h>


#if WITH_EDITOR
#	include <Editor.h>
#	include <Tests/AutomationEditorPromotionCommon.h>
#endif

////////////////////////////////////////////////////////////////
// DEFINITIONS

namespace Automatron
{
	class FTestSpecBase;

	struct FTestWorldSettings
	{
		TSubclassOf<UGameInstance> GameInstance;
		TSubclassOf<AGameModeBase> GameMode = AGameModeBase::StaticClass();

		bool bShouldTick = false;
	};

	namespace Spec
	{
		class FRegister
		{
		public:
			DECLARE_EVENT(FRegister, FOnSetup);

			static FOnSetup& OnSetup()
			{
				static FOnSetup Delegate{};
				return Delegate;
			}
		};

		/////////////////////////////////////////////////////
		// Initializes an spec instance at global execution time
		// and registers it to the system
		template <typename T>
		class TRegister : public FRegister
		{
		public:
			// Just by existing, this instance will define the class and register the spec
			static TRegister<T> Instance;


			TRegister()
			{
				OnSetup().AddStatic(&TRegister<T>::Setup);
			}

		private:
			static void Setup()
			{
				static T Spec{};
				Spec.Setup();
			}
		};

		/////////////////////////////////////////////////////
		// Represents an instance of a test
		struct FContext
		{
		private:
			int32 Id = 0;

		public:
			FContext(int32 Id = 0) : Id(Id) {}

			FContext NextContext() const
			{
				return {Id + 1};
			}
			int32 GetId() const
			{
				return Id;
			}

			friend uint32 GetTypeHash(const FContext& Item)
			{
				return Item.Id;
			}
			bool operator==(const FContext& Other) const
			{
				return Id == Other.Id;
			}
			operator bool() const
			{
				return Id > 0;
			}
		};

		struct FIt
		{
			FString Description;
			FString Id;
			FString Filename;
			int32 LineNumber;
			TSharedRef<IAutomationLatentCommand> Command;

			FIt(FString InDescription, FString InId, FString InFilename, int32 InLineNumber,
				TSharedRef<IAutomationLatentCommand> InCommand)
				: Description(MoveTemp(InDescription))
				, Id(MoveTemp(InId))
				, Filename(MoveTemp(InFilename))
				, LineNumber(MoveTemp(InLineNumber))
				, Command(MoveTemp(InCommand))
			{}
		};
	};	  // namespace Spec

	namespace Commands
	{
		class FSingleExecuteLatent : public IAutomationLatentCommand
		{
		private:
			const FTestSpecBase& Spec;
			const TFunction<void()> Predicate;
			const bool bSkipIfErrored = false;

		public:
			FSingleExecuteLatent(
				const FTestSpecBase& InSpec, TFunction<void()> InPredicate, bool bInSkipIfErrored = false)
				: Spec(InSpec)
				, Predicate(MoveTemp(InPredicate))
				, bSkipIfErrored(bInSkipIfErrored)
			{}
			virtual ~FSingleExecuteLatent() {}

			virtual bool Update() override;
		};

		class FUntilDoneLatent : public IAutomationLatentCommand
		{
		private:
			FTestSpecBase& Spec;
			const TFunction<void(const FDoneDelegate&)> Predicate;
			const FTimespan Timeout;
			const bool bSkipIfErrored = false;

			bool bIsRunning = false;
			FThreadSafeBool bDone = false;
			FDateTime StartedRunning;

		public:
			FUntilDoneLatent(FTestSpecBase& InSpec, TFunction<void(const FDoneDelegate&)> InPredicate,
				const FTimespan& InTimeout, bool bInSkipIfErrored = false)
				: Spec(InSpec)
				, Predicate(MoveTemp(InPredicate))
				, Timeout(InTimeout)
				, bSkipIfErrored(bInSkipIfErrored)
			{}
			virtual ~FUntilDoneLatent() {}

			virtual bool Update() override;

		private:
			void Done()
			{
				bDone = true;
			}
			void Reset();
		};

		class FAsyncUntilDoneLatent : public IAutomationLatentCommand
		{
		private:
			FTestSpecBase& Spec;
			const EAsyncExecution Execution;
			const TFunction<void(const FDoneDelegate&)> Predicate;
			const FTimespan Timeout;
			const bool bSkipIfErrored = false;

			FThreadSafeBool bDone = false;
			TFuture<void> Future;
			FDateTime StartedRunning;

		public:
			FAsyncUntilDoneLatent(FTestSpecBase& InSpec, EAsyncExecution InExecution,
				TFunction<void(const FDoneDelegate&)> InPredicate, const FTimespan& InTimeout,
				bool bInSkipIfErrored = false)
				: Spec(InSpec)
				, Execution(InExecution)
				, Predicate(MoveTemp(InPredicate))
				, Timeout(InTimeout)
				, bSkipIfErrored(bInSkipIfErrored)
			{}
			virtual ~FAsyncUntilDoneLatent() {}

			virtual bool Update() override;

		private:
			void Done()
			{
				bDone = true;
			}
			void Reset();
		};

		class FAsyncLatent : public IAutomationLatentCommand
		{
		private:
			FTestSpecBase& Spec;
			const EAsyncExecution Execution;
			const TFunction<void()> Predicate;
			const FTimespan Timeout;
			const bool bSkipIfErrored = false;

			FThreadSafeBool bDone = false;
			FDateTime StartedRunning;
			TFuture<void> Future;

		public:
			FAsyncLatent(FTestSpecBase& InSpec, EAsyncExecution InExecution, TFunction<void()> InPredicate,
				const FTimespan& InTimeout, bool bInSkipIfErrored = false)
				: Spec(InSpec)
				, Execution(InExecution)
				, Predicate(MoveTemp(InPredicate))
				, Timeout(InTimeout)
				, bSkipIfErrored(bInSkipIfErrored)
			{}
			virtual ~FAsyncLatent() {}

			virtual bool Update() override;

		private:
			void Done()
			{
				bDone = true;
			}
			void Reset();
		};
	};	  // namespace Commands

	class FTestSpecBase : public FAutomationTestBase, public TSharedFromThis<FTestSpecBase>
	{
	private:
		struct FSpecDefinitionScope
		{
			FString Description;

			TArray<TSharedRef<IAutomationLatentCommand>> BeforeEach;
			TArray<TSharedRef<Spec::FIt>> It;
			TArray<TSharedRef<IAutomationLatentCommand>> AfterEach;

			TArray<TSharedRef<FSpecDefinitionScope>> Children;
		};

		struct FSpec
		{
			FString Id;
			FString Description;
			FString Filename;
			int32 LineNumber;
			TArray<TSharedRef<IAutomationLatentCommand>> Commands;
		};

	protected:
		/* The timespan for how long a block should be allowed to execute before
		 * giving up and failing the test */
		FTimespan DefaultTimeout = FTimespan::FromSeconds(30);

		/* Whether or not BeforeEach and It blocks should skip execution if the test
		 * has already failed */
		bool bEnableSkipIfError = true;

	private:
		TArray<FString> Description;

		TMap<FString, TSharedRef<FSpec>> IdToSpecMap;

		TSharedPtr<FSpecDefinitionScope> RootDefinitionScope;

		TArray<TSharedRef<FSpecDefinitionScope>> DefinitionScopeStack;

		bool bHasBeenDefined = false;

		int32 TestsRemaining = 0;

		// The context of the active test
		Spec::FContext CurrentContext;

	public:
		FTestSpecBase()
			: FAutomationTestBase("", false)
			, RootDefinitionScope(MakeShared<FSpecDefinitionScope>())
		{
			DefinitionScopeStack.Push(RootDefinitionScope.ToSharedRef());
		}

		virtual ~FTestSpecBase() {}

		virtual bool RunTest(const FString& InParameters) override;

		virtual bool IsStressTest() const
		{
			return false;
		}
		virtual uint32 GetRequiredDeviceNum() const override
		{
			return 1;
		}

		virtual FString GetTestSourceFileName() const override;
		virtual int32 GetTestSourceFileLine() const override;
		virtual FString GetTestSourceFileName(const FString& InTestName) const override;
		virtual int32 GetTestSourceFileLine(const FString& InTestName) const override;

		virtual void GetTests(
			TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const override;

		// BEGIN Enabled Scopes
		void Describe(const FString& InDescription, TFunction<void()> DoWork);

		void It(const FString& InDescription, TFunction<void()> DoWork)
		{
			const TSharedRef<FSpecDefinitionScope> CurrentScope = DefinitionScopeStack.Last();
			const TArray<FProgramCounterSymbolInfo> Stack = FPlatformStackWalk::GetStack(1, 1);

			PushDescription(InDescription);
			auto Command = MakeShared<Commands::FSingleExecuteLatent>(*this, DoWork, bEnableSkipIfError);
			CurrentScope->It.Push(MakeShared<Spec::FIt>(
				GetDescription(), GetId(), Stack[0].Filename, Stack[0].LineNumber, Command));
			PopDescription(InDescription);
		}

		void It(const FString& InDescription, EAsyncExecution Execution, const FTimespan& Timeout,
			TFunction<void()> DoWork)
		{
			const TSharedRef<FSpecDefinitionScope> CurrentScope = DefinitionScopeStack.Last();
			const TArray<FProgramCounterSymbolInfo> Stack = FPlatformStackWalk::GetStack(1, 1);

			PushDescription(InDescription);
			auto Command =
				MakeShared<Commands::FAsyncLatent>(*this, Execution, DoWork, Timeout, bEnableSkipIfError);
			CurrentScope->It.Push(MakeShared<Spec::FIt>(
				GetDescription(), GetId(), Stack[0].Filename, Stack[0].LineNumber, Command));
			PopDescription(InDescription);
		}

		void It(const FString& InDescription, EAsyncExecution Execution, TFunction<void()> DoWork)
		{
			It(InDescription, Execution, DefaultTimeout, DoWork);
		}

		void LatentIt(const FString& InDescription, const FTimespan& Timeout,
			TFunction<void(const FDoneDelegate&)> DoWork)
		{
			const TSharedRef<FSpecDefinitionScope> CurrentScope = DefinitionScopeStack.Last();
			const TArray<FProgramCounterSymbolInfo> Stack = FPlatformStackWalk::GetStack(1, 1);

			PushDescription(InDescription);
			auto Command = MakeShared<Commands::FUntilDoneLatent>(*this, DoWork, Timeout, bEnableSkipIfError);
			CurrentScope->It.Push(MakeShared<Spec::FIt>(
				GetDescription(), GetId(), Stack[0].Filename, Stack[0].LineNumber, Command));
			PopDescription(InDescription);
		}

		void LatentIt(const FString& InDescription, TFunction<void(const FDoneDelegate&)> DoWork)
		{
			LatentIt(InDescription, DefaultTimeout, DoWork);
		}

		void LatentIt(const FString& InDescription, EAsyncExecution Execution, const FTimespan& Timeout,
			TFunction<void(const FDoneDelegate&)> DoWork)
		{
			const TSharedRef<FSpecDefinitionScope> CurrentScope = DefinitionScopeStack.Last();
			const TArray<FProgramCounterSymbolInfo> Stack = FPlatformStackWalk::GetStack(1, 1);

			PushDescription(InDescription);
			auto Command = MakeShared<Commands::FAsyncUntilDoneLatent>(
				*this, Execution, DoWork, Timeout, bEnableSkipIfError);
			CurrentScope->It.Push(MakeShared<Spec::FIt>(
				GetDescription(), GetId(), Stack[0].Filename, Stack[0].LineNumber, Command));
			PopDescription(InDescription);
		}

		void LatentIt(const FString& InDescription, EAsyncExecution Execution,
			TFunction<void(const FDoneDelegate&)> DoWork)
		{
			LatentIt(InDescription, Execution, DefaultTimeout, DoWork);
		}

		void BeforeEach(TFunction<void()> DoWork)
		{
			DefinitionScopeStack.Last()->BeforeEach.Push(
				MakeShared<Commands::FSingleExecuteLatent>(*this, DoWork, bEnableSkipIfError));
		}

		void BeforeEach(EAsyncExecution Execution, const FTimespan& Timeout, TFunction<void()> DoWork)
		{
			DefinitionScopeStack.Last()->BeforeEach.Push(
				MakeShared<Commands::FAsyncLatent>(*this, Execution, DoWork, Timeout, bEnableSkipIfError));
		}

		void BeforeEach(EAsyncExecution Execution, TFunction<void()> DoWork)
		{
			BeforeEach(Execution, DefaultTimeout, DoWork);
		}

		void LatentBeforeEach(const FTimespan& Timeout, TFunction<void(const FDoneDelegate&)> DoWork)
		{
			DefinitionScopeStack.Last()->BeforeEach.Push(
				MakeShared<Commands::FUntilDoneLatent>(*this, DoWork, Timeout, bEnableSkipIfError));
		}

		void LatentBeforeEach(TFunction<void(const FDoneDelegate&)> DoWork)
		{
			LatentBeforeEach(DefaultTimeout, DoWork);
		}

		void LatentBeforeEach(
			EAsyncExecution Execution, const FTimespan& Timeout, TFunction<void(const FDoneDelegate&)> DoWork)
		{
			DefinitionScopeStack.Last()->BeforeEach.Push(MakeShared<Commands::FAsyncUntilDoneLatent>(
				*this, Execution, DoWork, Timeout, bEnableSkipIfError));
		}

		void LatentBeforeEach(EAsyncExecution Execution, TFunction<void(const FDoneDelegate&)> DoWork)
		{
			LatentBeforeEach(Execution, DefaultTimeout, DoWork);
		}

		void AfterEach(TFunction<void()> DoWork)
		{
			DefinitionScopeStack.Last()->AfterEach.Push(
				MakeShared<Commands::FSingleExecuteLatent>(*this, DoWork));
		}

		void AfterEach(EAsyncExecution Execution, const FTimespan& Timeout, TFunction<void()> DoWork)
		{
			DefinitionScopeStack.Last()->AfterEach.Push(
				MakeShared<Commands::FAsyncLatent>(*this, Execution, DoWork, Timeout));
		}

		void AfterEach(EAsyncExecution Execution, TFunction<void()> DoWork)
		{
			AfterEach(Execution, DefaultTimeout, DoWork);
		}

		void LatentAfterEach(const FTimespan& Timeout, TFunction<void(const FDoneDelegate&)> DoWork)
		{
			DefinitionScopeStack.Last()->AfterEach.Push(
				MakeShared<Commands::FUntilDoneLatent>(*this, DoWork, Timeout));
		}

		void LatentAfterEach(TFunction<void(const FDoneDelegate&)> DoWork)
		{
			LatentAfterEach(DefaultTimeout, DoWork);
		}

		void LatentAfterEach(
			EAsyncExecution Execution, const FTimespan& Timeout, TFunction<void(const FDoneDelegate&)> DoWork)
		{
			DefinitionScopeStack.Last()->AfterEach.Push(
				MakeShared<Commands::FAsyncUntilDoneLatent>(*this, Execution, DoWork, Timeout));
		}

		void LatentAfterEach(EAsyncExecution Execution, TFunction<void(const FDoneDelegate&)> DoWork)
		{
			LatentAfterEach(Execution, DefaultTimeout, DoWork);
		}
		// END Enabled Scopes

		// BEGIN Disabled Scopes
		void xDescribe(const FString& InDescription, TFunction<void()> DoWork) {}

		void xIt(const FString& InDescription, TFunction<void()> DoWork) {}
		void xIt(const FString& InDescription, EAsyncExecution Execution, TFunction<void()> DoWork) {}
		void xIt(const FString& InDescription, EAsyncExecution Execution, const FTimespan& Timeout,
			TFunction<void()> DoWork)
		{}

		void xLatentIt(const FString& InDescription, TFunction<void(const FDoneDelegate&)> DoWork) {}
		void xLatentIt(const FString& InDescription, const FTimespan& Timeout,
			TFunction<void(const FDoneDelegate&)> DoWork)
		{}
		void xLatentIt(const FString& InDescription, EAsyncExecution Execution,
			TFunction<void(const FDoneDelegate&)> DoWork)
		{}
		void xLatentIt(const FString& InDescription, EAsyncExecution Execution, const FTimespan& Timeout,
			TFunction<void(const FDoneDelegate&)> DoWork)
		{}

		void xBeforeEach(TFunction<void()> DoWork) {}
		void xBeforeEach(EAsyncExecution Execution, TFunction<void()> DoWork) {}
		void xBeforeEach(EAsyncExecution Execution, const FTimespan& Timeout, TFunction<void()> DoWork) {}

		void xLatentBeforeEach(TFunction<void(const FDoneDelegate&)> DoWork) {}
		void xLatentBeforeEach(const FTimespan& Timeout, TFunction<void(const FDoneDelegate&)> DoWork) {}
		void xLatentBeforeEach(EAsyncExecution Execution, TFunction<void(const FDoneDelegate&)> DoWork) {}
		void xLatentBeforeEach(
			EAsyncExecution Execution, const FTimespan& Timeout, TFunction<void(const FDoneDelegate&)> DoWork)
		{}

		void xAfterEach(TFunction<void()> DoWork) {}
		void xAfterEach(EAsyncExecution Execution, TFunction<void()> DoWork) {}
		void xAfterEach(EAsyncExecution Execution, const FTimespan& Timeout, TFunction<void()> DoWork) {}

		void xLatentAfterEach(TFunction<void(const FDoneDelegate&)> DoWork) {}
		void xLatentAfterEach(const FTimespan& Timeout, TFunction<void(const FDoneDelegate&)> DoWork) {}
		void xLatentAfterEach(EAsyncExecution Execution, TFunction<void(const FDoneDelegate&)> DoWork) {}
		void xLatentAfterEach(
			EAsyncExecution Execution, const FTimespan& Timeout, TFunction<void(const FDoneDelegate&)> DoWork)
		{}
		// END Disabled Scopes

		int32 GetNumTests() const
		{
			return IdToSpecMap.Num();
		}
		int32 GetTestsRemaining() const
		{
			return GetNumTests() - CurrentContext.GetId();
		}
		Spec::FContext GetCurrentContext() const
		{
			return CurrentContext;
		}
		bool IsFirstTest() const
		{
			return CurrentContext.GetId() == 1;
		}
		bool IsLastTest() const
		{
			return CurrentContext.GetId() == GetNumTests();
		}

	protected:
		void EnsureDefinitions() const;

		virtual void RunDefine()
		{
			PreDefine();
			Define();
			PostDefine();
		}
		virtual void PreDefine();
		virtual void Define() = 0;
		virtual void PostDefine();

		void BakeDefinitions();

		void Redefine();

	private:
		void PushDescription(const FString& InDescription)
		{
			Description.Add(InDescription);
		}

		void PopDescription(const FString& InDescription)
		{
			Description.RemoveAt(Description.Num() - 1);
		}

		FString GetDescription() const;

		FString GetId() const;
	};

	class FTestSpec : public FTestSpecBase
	{
	public:
		// Should a world be initialized?
		bool bUseWorld = true;

		// If true the world used for testing will be reused for all tests
		bool bReuseWorldForAllTests = true;

		// If true and in editor, a PIE instance will be used to test
		bool bCanUsePIEWorld = true;

		FTestWorldSettings DefaultWorldSettings;

	private:
		FString ClassName;
		FString PrettyName;
		FString FileName;
		int32 LineNumber = -1;
		uint32 Flags = 0;

		bool bInitializedWorld = false;
#if WITH_EDITOR
		bool bInitializedPIE = false;
		FDelegateHandle PIEStartedHandle;
#endif

		TWeakObjectPtr<UWorld> MainWorld;

	public:
		FTestSpec() : FTestSpecBase() {}

		virtual FString GetTestSourceFileName() const override
		{
			return FileName;
		}
		virtual int32 GetTestSourceFileLine() const override
		{
			return LineNumber;
		}
		virtual uint32 GetTestFlags() const override
		{
			return Flags;
		}

		const FString& GetClassName() const
		{
			return ClassName;
		}
		const FString& GetPrettyName() const
		{
			return PrettyName;
		}

	protected:
		virtual FString GetBeautifiedTestName() const override
		{
			return PrettyName;
		}

		template <uint32 TFlags>
		void Setup(FString&& InName, FString&& InPrettyName, FString&& InFileName, int32 InLineNumber);

		// Used to indicate a test is pending to be implemented.
		void TestNotImplemented()
		{
			AddWarning(TEXT("Test not implemented"), 1);
		}

		virtual void PreDefine() override;
		virtual void PostDefine() override;

		void PrepareTestWorld(TFunction<void(UWorld* World)> OnWorldReady);

		void ReleaseTestWorld(UWorld* World);

		// Creates an empty world from scratch
		// @return world that was created
		UWorld* CreateWorld(FTestWorldSettings Settings = {});

		void TickWorldUntil(UWorld* World, bool bUseRealtime, TFunction<bool(float)> Delegate);
		void TickWorld(UWorld* World, float Duration, bool bUseRealtime = false);

		UGameInstance* CreateGameInstance(const FTestWorldSettings& Settings, UObject* Context);

		bool DestroyWorld(UWorld* World);

		UWorld* GetMainWorld() const
		{
			return MainWorld.Get();
		}

	private:
		void Reregister(const FString& NewName)
		{
			FAutomationTestFramework::Get().UnregisterAutomationTest(TestName);
			TestName = NewName;
			FAutomationTestFramework::Get().RegisterAutomationTest(TestName, this);
		}

		// Finds the first available game world (Standalone or PIE)
		static UWorld* FindGameWorld();

		static bool SetGameMode(UWorld* World, FTestWorldSettings& Settings);
	};

	namespace Spec
	{
		template <typename T>
		TRegister<T> TRegister<T>::Instance{};
	}

	static void RegisterSpecs()
	{
		Spec::FRegister::OnSetup().Broadcast();
	}
}	 // namespace Automatron


////////////////////////////////////////////////////////////////
// GENERATION MACROS

#define GENERATE_SPEC(TClass, PrettyName, TFlags) \
	GENERATE_SPEC_PRIVATE(TClass, PrettyName, TFlags, __FILE__, __LINE__)

#define GENERATE_SPEC_PRIVATE(TClass, PrettyName, TFlags, FileName, LineNumber)          \
private:                                                                                 \
	void Setup()                                                                         \
	{                                                                                    \
		FTestSpec::Setup<TFlags>(TEXT(#TClass), TEXT(PrettyName), FileName, LineNumber); \
	}                                                                                    \
	static Automatron::Spec::TRegister<TClass>& __meta_register()                        \
	{                                                                                    \
		return Automatron::Spec::TRegister<TClass>::Instance;                            \
	}                                                                                    \
	friend Automatron::Spec::TRegister<TClass>;                                          \
                                                                                         \
	virtual void Define() override

#define SPEC(TClass, TParent, PrettyName, TFlags)  \
	class TClass : public TParent                  \
	{                                              \
		GENERATE_SPEC(TClass, PrettyName, TFlags); \
	};                                             \
	void TClass::Define()


////////////////////////////////////////////////////////////////
// DECLARATIONS

namespace Automatron
{
	namespace Commands
	{
		inline bool FSingleExecuteLatent::Update()
		{
			if (bSkipIfErrored && Spec.HasAnyErrors())
			{
				return true;
			}

			Predicate();
			return true;
		}

		inline bool FUntilDoneLatent::Update()
		{
			if (!bIsRunning)
			{
				if (bSkipIfErrored && Spec.HasAnyErrors())
				{
					return true;
				}

				Predicate(FDoneDelegate::CreateSP(this, &FUntilDoneLatent::Done));
				bIsRunning = true;
				StartedRunning = FDateTime::UtcNow();
			}

			if (bDone)
			{
				Reset();
				return true;
			}
			else if (FDateTime::UtcNow() >= StartedRunning + Timeout)
			{
				Reset();
				Spec.AddError(TEXT("Latent command timed out."), 0);
				return true;
			}

			return false;
		}

		inline void FUntilDoneLatent::Reset()
		{
			// Reset the done for the next potential run of this command
			bDone = false;
			bIsRunning = false;
		}

		inline bool FAsyncUntilDoneLatent::Update()
		{
			if (!Future.IsValid())
			{
				if (bSkipIfErrored && Spec.HasAnyErrors())
				{
					return true;
				}

				Future = Async(Execution, [this]() {
					Predicate(FDoneDelegate::CreateRaw(this, &FAsyncUntilDoneLatent::Done));
				});

				StartedRunning = FDateTime::UtcNow();
			}

			if (bDone)
			{
				Reset();
				return true;
			}
			else if (FDateTime::UtcNow() >= StartedRunning + Timeout)
			{
				Reset();
				Spec.AddError(TEXT("Latent command timed out."), 0);
				return true;
			}
			return false;
		}

		inline void FAsyncUntilDoneLatent::Reset()
		{
			// Reset the done for the next potential run of this command
			bDone = false;
			Future = TFuture<void>();
		}

		inline bool FAsyncLatent::Update()
		{
			if (!Future.IsValid())
			{
				if (bSkipIfErrored && Spec.HasAnyErrors())
				{
					return true;
				}

				Future = Async(Execution, [this]() {
					Predicate();
					bDone = true;
				});

				StartedRunning = FDateTime::UtcNow();
			}

			if (bDone)
			{
				Reset();
				return true;
			}
			else if (FDateTime::UtcNow() >= StartedRunning + Timeout)
			{
				Reset();
				Spec.AddError(TEXT("Latent command timed out."), 0);
				return true;
			}

			return false;
		}

		inline void FAsyncLatent::Reset()
		{
			// Reset the done for the next potential run of this command
			bDone = false;
			Future = TFuture<void>();
		}
	}	 // namespace Commands

	inline void FTestSpecBase::EnsureDefinitions() const
	{
		if (!bHasBeenDefined)
		{
			const_cast<FTestSpecBase*>(this)->RunDefine();
			const_cast<FTestSpecBase*>(this)->BakeDefinitions();
		}
	}

	inline FString FTestSpecBase::GetTestSourceFileName() const
	{
		return FAutomationTestBase::GetTestSourceFileName();
	}

	inline int32 FTestSpecBase::GetTestSourceFileLine() const
	{
		return FAutomationTestBase::GetTestSourceFileLine();
	}

	inline bool FTestSpecBase::RunTest(const FString& InParameters)
	{
		EnsureDefinitions();

		if (!InParameters.IsEmpty())
		{
			const TSharedRef<FSpec>* SpecToRun = IdToSpecMap.Find(InParameters);
			if (SpecToRun != nullptr)
			{
				for (int32 Index = 0; Index < (*SpecToRun)->Commands.Num(); ++Index)
				{
					FAutomationTestFramework::GetInstance().EnqueueLatentCommand(
						(*SpecToRun)->Commands[Index]);
				}
			}
		}
		else
		{
			TArray<TSharedRef<FSpec>> Specs;
			IdToSpecMap.GenerateValueArray(Specs);

			for (int32 SpecIndex = 0; SpecIndex < Specs.Num(); SpecIndex++)
			{
				for (int32 CommandIndex = 0; CommandIndex < Specs[SpecIndex]->Commands.Num(); ++CommandIndex)
				{
					FAutomationTestFramework::GetInstance().EnqueueLatentCommand(
						Specs[SpecIndex]->Commands[CommandIndex]);
				}
			}
		}

		TestsRemaining = GetNumTests();
		return true;
	}

	inline FString FTestSpecBase::GetTestSourceFileName(const FString& InTestName) const
	{
		FString TestId = InTestName;
		if (TestId.StartsWith(TestName + TEXT(" ")))
		{
			TestId = InTestName.RightChop(TestName.Len() + 1);
		}

		const TSharedRef<FSpec>* Spec = IdToSpecMap.Find(TestId);
		if (Spec != nullptr)
		{
			return (*Spec)->Filename;
		}

		return GetTestSourceFileName();
	}

	inline int32 FTestSpecBase::GetTestSourceFileLine(const FString& InTestName) const
	{
		FString TestId = InTestName;
		if (TestId.StartsWith(TestName + TEXT(" ")))
		{
			TestId = InTestName.RightChop(TestName.Len() + 1);
		}

		const TSharedRef<FSpec>* Spec = IdToSpecMap.Find(TestId);
		if (Spec != nullptr)
		{
			return (*Spec)->LineNumber;
		}

		return GetTestSourceFileLine();
	}

	inline void FTestSpecBase::GetTests(
		TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
	{
		EnsureDefinitions();

		TArray<TSharedRef<FSpec>> Specs;
		IdToSpecMap.GenerateValueArray(Specs);

		for (int32 Index = 0; Index < Specs.Num(); Index++)
		{
			OutTestCommands.Push(Specs[Index]->Id);
			OutBeautifiedNames.Push(Specs[Index]->Description);
		}
	}

	inline void FTestSpecBase::Describe(const FString& InDescription, TFunction<void()> DoWork)
	{
		const TSharedRef<FSpecDefinitionScope> ParentScope = DefinitionScopeStack.Last();
		const TSharedRef<FSpecDefinitionScope> NewScope = MakeShared<FSpecDefinitionScope>();
		NewScope->Description = InDescription;
		ParentScope->Children.Push(NewScope);

		DefinitionScopeStack.Push(NewScope);
		PushDescription(InDescription);
		DoWork();
		PopDescription(InDescription);
		DefinitionScopeStack.Pop();

		if (NewScope->It.Num() == 0 && NewScope->Children.Num() == 0)
		{
			ParentScope->Children.Remove(NewScope);
		}
	}

	inline void FTestSpecBase::PreDefine()
	{
		BeforeEach([this]() {
			CurrentContext = CurrentContext.NextContext();
		});
	}

	inline void FTestSpecBase::PostDefine()
	{
		AfterEach([this]() {
			if (IsLastTest())
			{
				CurrentContext = {};
			}
		});
	}

	inline void FTestSpecBase::BakeDefinitions()
	{
		TArray<TSharedRef<FSpecDefinitionScope>> Stack;
		Stack.Push(RootDefinitionScope.ToSharedRef());

		TArray<TSharedRef<IAutomationLatentCommand>> BeforeEach;
		TArray<TSharedRef<IAutomationLatentCommand>> AfterEach;

		while (Stack.Num() > 0)
		{
			const TSharedRef<FSpecDefinitionScope> Scope = Stack.Last();

			BeforeEach.Append(Scope->BeforeEach);
			// ScopeAfter each are added reversed
			AfterEach.Reserve(AfterEach.Num() + Scope->AfterEach.Num());
			for (int32 i = Scope->AfterEach.Num() - 1; i >= 0; --i)
			{
				AfterEach.Add(Scope->AfterEach[i]);
			}

			for (int32 ItIndex = 0; ItIndex < Scope->It.Num(); ItIndex++)
			{
				TSharedRef<Spec::FIt> It = Scope->It[ItIndex];

				TSharedRef<FSpec> Spec = MakeShared<FSpec>();
				Spec->Id = It->Id;
				Spec->Description = It->Description;
				Spec->Filename = It->Filename;
				Spec->LineNumber = It->LineNumber;
				Spec->Commands.Append(BeforeEach);
				Spec->Commands.Add(It->Command);

				// Add after each reversed
				for (int32 i = AfterEach.Num() - 1; i >= 0; --i)
				{
					Spec->Commands.Add(AfterEach[i]);
				}

				check(!IdToSpecMap.Contains(Spec->Id));
				IdToSpecMap.Add(Spec->Id, Spec);
			}
			Scope->It.Empty();

			if (Scope->Children.Num() > 0)
			{
				Stack.Append(Scope->Children);
				Scope->Children.Empty();
			}
			else
			{
				while (Stack.Num() > 0 && Stack.Last()->Children.Num() == 0 && Stack.Last()->It.Num() == 0)
				{
					const TSharedRef<FSpecDefinitionScope> PoppedScope = Stack.Pop();

					if (PoppedScope->BeforeEach.Num() > 0)
					{
						BeforeEach.RemoveAt(
							BeforeEach.Num() - PoppedScope->BeforeEach.Num(), PoppedScope->BeforeEach.Num());
					}

					if (PoppedScope->AfterEach.Num() > 0)
					{
						AfterEach.RemoveAt(
							AfterEach.Num() - PoppedScope->AfterEach.Num(), PoppedScope->AfterEach.Num());
					}
				}
			}
		}

		RootDefinitionScope.Reset();
		DefinitionScopeStack.Reset();
		bHasBeenDefined = true;
	}

	inline void FTestSpecBase::Redefine()
	{
		Description.Empty();
		IdToSpecMap.Empty();
		RootDefinitionScope.Reset();
		DefinitionScopeStack.Empty();
		bHasBeenDefined = false;
	}

	inline FString FTestSpecBase::GetDescription() const
	{
		FString CompleteDescription;
		for (int32 Index = 0; Index < Description.Num(); ++Index)
		{
			if (Description[Index].IsEmpty())
			{
				continue;
			}

			if (CompleteDescription.IsEmpty())
			{
				CompleteDescription = Description[Index];
			}
			else if (FChar::IsWhitespace(CompleteDescription[CompleteDescription.Len() - 1]) ||
					 FChar::IsWhitespace(Description[Index][0]))
			{
				CompleteDescription = CompleteDescription + TEXT(".") + Description[Index];
			}
			else
			{
				CompleteDescription =
					FString::Printf(TEXT("%s.%s"), *CompleteDescription, *Description[Index]);
			}
		}

		return CompleteDescription;
	}

	inline FString FTestSpecBase::GetId() const
	{
		if (Description.Last().EndsWith(TEXT("]")))
		{
			FString ItDescription = Description.Last();
			ItDescription.RemoveAt(ItDescription.Len() - 1);

			int32 StartingBraceIndex = INDEX_NONE;
			if (ItDescription.FindLastChar(TEXT('['), StartingBraceIndex) &&
				StartingBraceIndex != ItDescription.Len() - 1)
			{
				FString CommandId = ItDescription.RightChop(StartingBraceIndex + 1);
				return CommandId;
			}
		}

		FString CompleteId;
		for (int32 Index = 0; Index < Description.Num(); ++Index)
		{
			if (Description[Index].IsEmpty())
			{
				continue;
			}

			if (CompleteId.IsEmpty())
			{
				CompleteId = Description[Index];
			}
			else if (FChar::IsWhitespace(CompleteId[CompleteId.Len() - 1]) ||
					 FChar::IsWhitespace(Description[Index][0]))
			{
				CompleteId = CompleteId + Description[Index];
			}
			else
			{
				CompleteId = FString::Printf(TEXT("%s %s"), *CompleteId, *Description[Index]);
			}
		}

		return CompleteId;
	}

	inline void FTestSpec::PreDefine()
	{
		FTestSpecBase::PreDefine();

		if (!bUseWorld)
		{
			return;
		}

		LatentBeforeEach(EAsyncExecution::TaskGraphMainThread, [this](const auto Done) {
			PrepareTestWorld([this, Done](UWorld* InWorld) {
				MainWorld = InWorld;
				Done.Execute();
			});
		});
	}

	inline void FTestSpec::PostDefine()
	{
		AfterEach([this]() {
			// If this spec initialized a PIE world, tear it down
			if (!bReuseWorldForAllTests || IsLastTest())
			{
				ReleaseTestWorld(MainWorld.Get());
			}
		});

		FTestSpecBase::PostDefine();
	}

	inline void FTestSpec::PrepareTestWorld(TFunction<void(UWorld* World)> OnWorldReady)
	{
		checkf(IsInGameThread(), TEXT("PrepareTestWorld can only be run on game thread."));

		UWorld* SelectedWorld = FindGameWorld();

#if WITH_EDITOR
		// If there was no PIE world, start it and try again
		if (bCanUsePIEWorld && !SelectedWorld && GIsEditor)
		{
			PIEStartedHandle =
				FEditorDelegates::PostPIEStarted.AddLambda([this, OnWorldReady](const bool bIsSimulating) {
					UWorld* SelectedWorld = FindGameWorld();
					bInitializedPIE = SelectedWorld != nullptr;
					bInitializedWorld = bInitializedPIE;

					OnWorldReady(SelectedWorld);
				});
			FEditorPromotionTestUtilities::StartPIE(false);
			return;
		}
#endif

		if (!SelectedWorld)
		{
			SelectedWorld = CreateWorld(DefaultWorldSettings);
			bInitializedWorld = true;
		}

		OnWorldReady(SelectedWorld);
	}

	inline void FTestSpec::ReleaseTestWorld(UWorld* World)
	{
		if (!IsInGameThread())
		{
			AsyncTask(ENamedThreads::GameThread, [this, World]() {
				ReleaseTestWorld(World);
			});
			return;
		}

#if WITH_EDITOR
		FEditorDelegates::PostPIEStarted.Remove(PIEStartedHandle);
		if (bInitializedPIE)
		{
			FEditorPromotionTestUtilities::EndPIE();
			bInitializedPIE = false;
			bInitializedWorld = false;
			return;
		}
#endif

		if (!bInitializedWorld)
		{
			return;
		}

		// If world is not PIE, we take care of its teardown
		DestroyWorld(World);
		bInitializedWorld = false;
	}

	inline UWorld* FTestSpec::CreateWorld(FTestWorldSettings Settings)
	{
		auto* GameInstance = CreateGameInstance(Settings, GEngine);
		GameInstance->AddToRoot();

		GameInstance->InitializeStandalone(TEXT("FAbilitySpec::World"), nullptr);
		UWorld* World = GameInstance->GetWorld();

		const bool bInformEngineOfWorld = true;
		if (GEngine && bInformEngineOfWorld)
		{
			GEngine->WorldAdded(World);
		}

		World->SetShouldTick(Settings.bShouldTick);
		SetGameMode(World, Settings);

		FURL URL;
		World->InitializeActorsForPlay(URL);
		World->BeginPlay();

		World->AddToRoot();
		return World;
	}

	inline void FTestSpec::TickWorldUntil(UWorld* World, bool bUseRealtime, TFunction<bool(float)> Delegate)
	{
		check(IsInGameThread());

		const float Step = 1.f / 60.f;	  // 60 fps

		if (!bUseRealtime)
		{
			while (IsValid(World) && Delegate(Step))
			{
				World->Tick(ELevelTick::LEVELTICK_All, Step);

				// This is terrible but required for subticking like this.
				// we could always cache the real GFrameCounter at the start of our tests
				// and restore it when finished.
				++GFrameCounter;
			}
			return;
		}

		float DeltaTime = Step;
		while (IsValid(World) && Delegate(DeltaTime))
		{
			int32 TickCycles = 0;
			CLOCK_CYCLES(TickCycles);
			World->Tick(ELevelTick::LEVELTICK_All, DeltaTime);
			UNCLOCK_CYCLES(TickCycles);

			// This is terrible but required for subticking like this.
			// we could always cache the real GFrameCounter at the start of our tests
			// and restore it when finished.
			++GFrameCounter;

			const float TickDuration = FPlatformTime::ToSeconds(TickCycles);
			if (TickDuration < Step)
			{
				FPlatformProcess::Sleep(Step - TickDuration);
			}

			DeltaTime = FMath::Max(TickDuration, Step);
		}
	}

	inline void FTestSpec::TickWorld(UWorld* World, float Duration, bool bUseRealtime)
	{
		TickWorldUntil(World, bUseRealtime, [&Duration](float DeltaTime) {
			Duration -= DeltaTime;
			return Duration > 0.f;
		});
	}

	inline UGameInstance* FTestSpec::CreateGameInstance(const FTestWorldSettings& Settings, UObject* Context)
	{
		UClass* GameInstanceClass = Settings.GameInstance.Get();
		if (!GameInstanceClass)
		{
			FSoftClassPath GameInstanceClassName = GetDefault<UGameMapsSettings>()->GameInstanceClass;
			GameInstanceClass = GameInstanceClassName.TryLoadClass<UGameInstance>();
		}

		if (!GameInstanceClass)
		{
			GameInstanceClass = UGameInstance::StaticClass();
		}
		return NewObject<UGameInstance>(Context, GameInstanceClass);
	}

	inline bool FTestSpec::DestroyWorld(UWorld* World)
	{
		// If world is not PIE, we take care of its teardown
		if (World && !World->IsPlayInEditor())
		{
			World->BeginTearingDown();

			// Cancel any pending connection to a server
			GEngine->CancelPending(World);

			// Shut down any existing game connections
			GEngine->ShutdownWorldNetDriver(World);

			for (FActorIterator ActorIt(World); ActorIt; ++ActorIt)
			{
				ActorIt->RouteEndPlay(EEndPlayReason::Quit);
			}

			if (auto* GameInstance = World->GetGameInstance())
			{
				GameInstance->Shutdown();
				GameInstance->RemoveFromRoot();
			}

			GEngine->DestroyWorldContext(World);
			World->DestroyWorld(false);

			return true;
		}
		return false;
	}

	inline UWorld* FTestSpec::FindGameWorld()
	{
		const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
		for (const FWorldContext& Context : WorldContexts)
		{
			if (Context.World() != nullptr)
			{
				if (Context.WorldType == EWorldType::PIE /*&& Context.PIEInstance == 0*/)
				{
					return Context.World();
				}

				if (Context.WorldType == EWorldType::Game)
				{
					return Context.World();
				}
			}
		}
		return nullptr;
	}

	inline bool FTestSpec::SetGameMode(UWorld* World, FTestWorldSettings& Settings)
	{
		if ((!World->IsNetMode(NM_DedicatedServer) && !World->IsNetMode(NM_ListenServer)) ||
			World->GetAuthGameMode())
		{
			return false;
		}

		if (!Settings.GameMode)
		{
			Settings.GameMode = AGameModeBase::StaticClass();
		}

		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnInfo.ObjectFlags |= RF_Transient;	  // We never want to save game modes into a map

		auto* GameMode = World->SpawnActor<AGameModeBase>(Settings.GameMode, SpawnInfo);
		World->CopyGameState(GameMode, nullptr);
		return GameMode != nullptr;
	}

	template <uint32 TFlags>
	inline void FTestSpec::Setup(
		FString&& InName, FString&& InPrettyName, FString&& InFileName, int32 InLineNumber)
	{
		static_assert(TFlags & EAutomationTestFlags::ApplicationContextMask,
			"AutomationTest has no application flag. It shouldn't run. See "
			"AutomationTest.h.");
		static_assert(
			((TFlags & EAutomationTestFlags::FilterMask) == EAutomationTestFlags::SmokeFilter) ||
				((TFlags & EAutomationTestFlags::FilterMask) == EAutomationTestFlags::EngineFilter) ||
				((TFlags & EAutomationTestFlags::FilterMask) == EAutomationTestFlags::ProductFilter) ||
				((TFlags & EAutomationTestFlags::FilterMask) == EAutomationTestFlags::PerfFilter) ||
				((TFlags & EAutomationTestFlags::FilterMask) == EAutomationTestFlags::StressFilter) ||
				((TFlags & EAutomationTestFlags::FilterMask) == EAutomationTestFlags::NegativeFilter),
			"All AutomationTests must have exactly 1 filter type "
			"specified.  See AutomationTest.h.");

		ClassName = InName;
		PrettyName = MoveTemp(InPrettyName);
		FileName = MoveTemp(InFileName);
		LineNumber = InLineNumber;
		Flags = TFlags;

		Reregister(InName);
	}
}	 // namespace Automatron
