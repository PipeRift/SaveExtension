// Copyright 2015-2020 Piperift. All Rights Reserved.

#pragma once

#include "Misc/TypeTraits.h"

#include <Async/AsyncWork.h>

class ITaskHolder
{
public:
	virtual bool Tick() = 0;
	virtual void Cancel(bool bFinishSynchronously) = 0;
	virtual ~ITaskHolder()
	{
	}
};

template <class TaskType>
class FTaskHolder : public FAsyncTask<TaskType>, public ITaskHolder
{
	using Super = FAsyncTask<TaskType>;

public:
	DECLARE_EVENT_OneParam(FTaskHolder<TaskType>, FFinishedEvent, FTaskHolder<TaskType>&);

	bool bNotified = false;
	FFinishedEvent _OnFinished;

	FTaskHolder() : ITaskHolder(), Super()
	{
	}
	virtual ~FTaskHolder()
	{
	}

	template <typename... ArgTypes>
	FTaskHolder(ArgTypes&&... CtrArgs) : Super(Forward<ArgTypes>(CtrArgs)...), ITaskHolder()
	{
	}

	auto& OnFinished(TFunction<void(FTaskHolder<TaskType>&)> Delegate)
	{
		_OnFinished.AddLambda(Delegate);
		return *this;
	}

	virtual bool Tick() override
	{
		if (Super::IsDone())
		{
			TryNotifyFinish();
			return true;
		}
		return false;
	}

	virtual void Cancel(bool bFinishSynchronously) override
	{
		if (!Super::IsIdle())
		{
			Super::EnsureCompletion(bFinishSynchronously);
			TryNotifyFinish();
		}
		else if (Super::IsDone())
		{
			TryNotifyFinish();
		}
	}

	TaskType* operator->()
	{
		return &Super::GetTask();
	}

private:

	void TryNotifyFinish()
	{
		if (!bNotified)
		{
			_OnFinished.Broadcast(*this);
			bNotified = true;
		}
	}
};

/** Manages the lifetime of many multi-threaded tasks */
class FScopedTaskList
{
	TArray<TUniquePtr<ITaskHolder>> Tasks;

public:
	FScopedTaskList()
	{
	}

	template <class TaskType, typename... ArgTypes>
	inline FTaskHolder<TaskType>& CreateTask(ArgTypes&&... CtrArgs)
	{
		auto NewTask = MakeUnique<FTaskHolder<TaskType>>(Forward<ArgTypes>(CtrArgs)...);
		auto* TaskPtr = NewTask.Get();
		Tasks.Add(MoveTemp(NewTask));
		return *TaskPtr;
	}

	void Tick()
	{
		// Tick all running tasks and remove the ones that finished
		Tasks.RemoveAllSwap([](auto& Task) { return Task->Tick(); });
	}

	void CancelAll()
	{
		for (auto& Task : Tasks)
		{
			Task->Cancel(true);
		}
		Tasks.Empty();
	}
};
