// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include <Async/AsyncWork.h>
#include "Misc/TypeTraits.h"


template<class TaskType>
class FTaskHolder : public FAsyncTask<TaskType> {
	using Super = FAsyncTask<TaskType>;

public:
	using Callback = TFunction<void(FTaskHolder<TaskType>&)>;

	Callback _OnFinished;


	FTaskHolder() : Super() {}

	template <typename... ArgTypes>
	FTaskHolder(ArgTypes&&... CtrArgs) : Super(Forward<ArgTypes>(CtrArgs)...) {}

	auto& OnFinished(Callback&& Delegate) {
		_OnFinished = Delegate;
		return *this;
	}

	TaskType* operator->() {
		return &Super::GetTask();
	}
};


/** Use TScopedTaskList. Manages the lifetime of an array of multi-threaded tasks */
template<class TaskType>
struct TScopedTaskListType {

	using Callback = TFunction<void(FTaskHolder<TaskType>&)>;

private:

	TArray<FTaskHolder<TaskType>*> Tasks;

public:

	TScopedTaskListType() : Tasks{} {}

	TScopedTaskListType(TScopedTaskListType&& other) {
		Tasks = MoveTemp(other.Tasks);
	}

	template <typename... ArgTypes>
	FTaskHolder<TaskType>& CreateTask(ArgTypes&&... CtrArgs)
	{
		auto* NewTask = new FTaskHolder<TaskType>(Forward<ArgTypes>(CtrArgs)...);
		Tasks.Add(NewTask);
		return *NewTask;
	}

	void Tick() {
		Tasks.RemoveAllSwap([this](auto* Task) {
			if (Task->IsDone())
			{
				Task->_OnFinished(*Task);
				delete Task;
				return true;
			}
			return false;
		});
	}

	void CancelAll()
	{
		for (auto* Task : Tasks)
		{
			if (!Task->IsIdle())
			{
				Task->EnsureCompletion(false);
				Task->_OnFinished(*Task);
			}

			delete Task;
		}
		Tasks.Empty();
	}

	~TScopedTaskListType() {
		CancelAll();
	}
};


/** Manages the lifetime of an array of multi-threaded tasks */
template<class ...Args>
struct TScopedTaskList {
	static constexpr uint32 TypesCount = sizeof...(Args);

	TTuple<TScopedTaskListType<Args>...> SingleManagers;


public:

	TScopedTaskList() {}
	TScopedTaskList(TScopedTaskList&& other) {}

	template<class Type, typename... ArgTypes>
	inline FTaskHolder<Type>& CreateTask(ArgTypes&&... CtrArgs) {
		static_assert(VariadicContainsType<Type, Args...>(), "Can't create a task of this type");

		constexpr uint32 I = GetVariadicTypeIndex<0, Type, Args...>();

		return SingleManagers.Get<I>().CreateTask(Forward<ArgTypes>(CtrArgs)...);
	}

	void Tick() {
		TickForEachId();
	}

	void CancelAll() {
		CancelForEachId();
	}

private:

	template<uint32 I = 0>
	void TickForEachId() {
		SingleManagers.Get<I>().Tick();
		TickForEachId<I + 1>();
	}

	template<>
	void TickForEachId<TypesCount>() {}

	template<uint32 I = 0>
	void CancelForEachId() {
		SingleManagers.Get<I>().CancelAll();
		CancelForEachId<I + 1>();
	}

	template<>
	void CancelForEachId<TypesCount>() {}
};
