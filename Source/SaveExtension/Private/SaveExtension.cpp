// Copyright 2015-2024 Piperift. All Rights Reserved.

#include "SaveExtension.h"


DEFINE_LOG_CATEGORY(LogSaveExtension)

IMPLEMENT_MODULE(FSaveExtension, SaveExtension);

void FSaveExtension::Log(const USaveSlot* Slot, const FString& Message, FColor Color, bool bError, const float Duration)
	{
		if (Slot->bDebug)
		{
			if (bError)
			{
				Color = FColor::Red;
			}

			const FString ComposedMessage{FString::Printf(TEXT("SE: %s"), *Message)};

			if (bError)
			{
				UE_LOG(LogSaveExtension, Error, TEXT("%s"), *ComposedMessage);
			}
			else
			{
				UE_LOG(LogSaveExtension, Log, TEXT("%s"), *ComposedMessage);
			}

			if (Slot->bDebugInScreen && GEngine)
			{
				GEngine->AddOnScreenDebugMessage(-1, Duration, Color, ComposedMessage);
			}
		}
	}
