// Copyright 2015-2020 Piperift. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "EdGraphUtilities.h"
#include "GameplayTagContainer.h"
#include "EdGraphSchema_K2.h"
#include "SGraphPin.h"
#include "SClassFilterGraphPin.h"
#include "Misc/ClassFilter.h"


class FClassFilterGraphPanelPinFactory: public FGraphPanelPinFactory
{
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override
	{
		if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
		{
			if (auto* PinStructType = Cast<UScriptStruct>(InPin->PinType.PinSubCategoryObject.Get()))
			{
				if (PinStructType->IsChildOf(FClassFilter::StaticStruct()))
				{
					return SNew(SClassFilterGraphPin, InPin);
				}
			}
		}

		return nullptr;
	}
};
