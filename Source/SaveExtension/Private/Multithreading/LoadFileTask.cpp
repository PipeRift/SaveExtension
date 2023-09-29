// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "Multithreading/LoadFileTask.h"


/////////////////////////////////////////////////////
// FLoadFileTask

FLoadFileTask::FLoadFileTask(USaveManager* Manager, USaveSlot* InLastSlot, FStringView SlotName)
    : Manager(Manager), SlotName(SlotName), LastSlot(InLastSlot)
{
    if (LastSlot.IsValid()) // If an slot was provided, that could be reused, mark it async
    {
        LastSlot->SetInternalFlags(EInternalObjectFlags::Async);
        LastSlotData = LastSlot->GetData();
        if (LastSlotData.IsValid())
        {
            LastSlotData->SetInternalFlags(EInternalObjectFlags::Async);
        }
    }
}

FLoadFileTask::~FLoadFileTask()
{
    if (Slot.IsValid())
    {
        Slot->ClearInternalFlags(EInternalObjectFlags::Async);
        if (USaveSlotData* SlotData = Slot->GetData())
        {
            SlotData->ClearInternalFlags(EInternalObjectFlags::Async);
        }
    }
    if (LastSlot.IsValid())
    {
        LastSlot->ClearInternalFlags(EInternalObjectFlags::Async);
    }
    if (LastSlotData.IsValid())
    {
        LastSlotData->ClearInternalFlags(EInternalObjectFlags::Async);
    }
}

void FLoadFileTask::DoWork()
{
    USaveSlot* NewSlot = LastSlot.Get();
    FSaveFileHelpers::LoadFile(SlotName, NewSlot, true, Manager.Get());
    Slot = NewSlot;
}
