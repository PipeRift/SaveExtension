// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include <Async/AsyncWork.h>
#include "Misc/TypeTraits.h"


class ITaskHolder {
public:
	virtual bool Tick() = 0;
	virtual void Cancel(bool bFinishSynchronously) = 0;
	virtual ~ITaskHolder() {}
};

template<class TaskType>
class FTaskHolder : public FAsyncTask<TaskType>, public ITaskHolder {
	using Super = FAsyncTask<TaskType>;

public:
	DECLARE_EVENT_OneParam(FTaskHolder<TaskType>, FFinishedEvent, FTaskHolder<TaskType>&);

	FFinishedEvent _OnFinished;


	FTaskHolder() : ITaskHolder(), Super() {}
	virtual ~FTaskHolder() {}

	template <typename... ArgTypes>
	FTaskHolder(ArgTypes&&... CtrArgs) : Super(Forward<ArgTypes>(CtrArgs)...), ITaskHolder() {}

	auto& OnFinished(TFunction<void(FTaskHolder<TaskType>&)> Delegate) {
		_OnFinished.AddLambda(Delegate);
		return *this;
	}

	virtual bool Tick() override {
		if (Super::IsDone())
		{
			_OnFinished.Broadcast(*this);
			return true;
		}
		return false;
	}

	virtual void Cancel(bool bFinishSynchronously) override {
		if (!Super::IsIdle())
		{
			Super::EnsureCompletion(bFinishSynchronously);
			_OnFinished.Broadcast(*this);
		}
	}

	TaskType* operator->() {
		return &Super::GetTask();
	}
};


/** Manages the lifetime of many multi-threaded tasks */
class FScopedTaskList {

	TArray<ITaskHolder*> Tasks;


public:

	FScopedTaskList() {}
	FScopedTaskList(FScopedTaskList&& other) {}

	template<class TaskType, typename... ArgTypes>
	inline FTaskHolder<TaskType>& CreateTask(ArgTypes&&... CtrArgs) {
		auto* NewTask = new FTaskHolder<TaskType>(Forward<ArgTypes>(CtrArgs)...);
		Tasks.Add(NewTask);
		return *NewTask;
	}

	void Tick() {
		// Tick all running tasks and remove the ones that finished
		Tasks.RemoveAllSwap([](auto* Task) {
			bool bFinished = Task->Tick();
			if (bFinished)
				delete Task;
			return bFinished;
		});
	}

	void CancelAll() {
		for (auto* Task : Tasks)
		{
			Task->Cancel(false);
			delete Task;
		}
		Tasks.Empty();
	}
};
