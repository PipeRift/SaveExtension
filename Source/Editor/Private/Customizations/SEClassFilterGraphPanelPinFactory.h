// Copyright 2015-2024 Piperift. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphUtilities.h"
#include "GameplayTagContainer.h"
#include "Misc/ClassFilter.h"
#include "SClassFilterGraphPin.h"
#include "SGraphPin.h"
#include "Widgets/DeclarativeSyntaxSupport.h"



class FSEClassFilterGraphPanelPinFactory : public FGraphPanelPinFactory
{
	virtual TSharedPtr<class SGraphPin> CreatePin(class UEdGraphPin* InPin) const override
	{
		if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct)
		{
			if (auto* PinStructType = Cast<UScriptStruct>(InPin->PinType.PinSubCategoryObject.Get()))
			{
				if (PinStructType->IsChildOf(FSEClassFilter::StaticStruct()))
				{
					return SNew(SClassFilterGraphPin, InPin);
				}
			}
		}

		return nullptr;
	}
};
