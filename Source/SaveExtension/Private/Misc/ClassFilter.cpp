// Copyright 2015-2019 Piperift. All Rights Reserved.

#include "ClassFilter.h"

#include <Engine/StaticMeshActor.h>
#include <Engine/ReflectionCapture.h>
#include <Engine/LODActor.h>
#include <Engine/Brush.h>
#include <GameFramework/GameMode.h>
#include <GameFramework/GameState.h>
#include <GameFramework/PlayerState.h>
#include <GameFramework/PlayerController.h>
#include <GameFramework/HUD.h>
#include <InstancedFoliageActor.h>
#include <Lightmass/LightmassPortal.h>
#include <NavigationData.h>
#include <Parse.h>


FClassFilter::FClassFilter(const UClass* BaseClass)
	: BaseClass{ BaseClass }
	, IgnoredClasses {
		/*AStaticMeshActor::StaticClass(),
		AInstancedFoliageActor::StaticClass(),
		AReflectionCapture::StaticClass(),
		APlayerController::StaticClass(),
		ALightmassPortal::StaticClass(),
		ANavigationData::StaticClass(),
		APlayerState::StaticClass(),
		AGameState::StaticClass(),
		AGameMode::StaticClass(),
		ALODActor::StaticClass(),
		ABrush::StaticClass(),
		AHUD::StaticClass()*/
	}
{}

void FClassFilter::BakeAllowedClasses()
{
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* const Class = *It;

		// Iterate parent classes of a class
		const UClass* CurrentClass = Class;
		while (CurrentClass)
		{
			if (AllowedClasses.Contains(CurrentClass))
			{
				// First parent allowed class marks it as allowed
				BakedAllowedClasses.Add(Class);
				break;
			}
			else if (IgnoredClasses.Contains(CurrentClass))
			{
				// First parent ignored class marks it as not allowed
				break;
			}
			CurrentClass = CurrentClass->GetSuperClass();
		}
	}
}

FString FClassFilter::ToString()
{
	FString ExportString;
	FClassFilter::StaticStruct()->ExportText(ExportString, this, this, nullptr, 0, nullptr);
	return ExportString;
}

void FClassFilter::FromString(FString String)
{
	if (String.StartsWith(TEXT("(")) && String.EndsWith(TEXT(")")))
	{
		String = String.LeftChop(1);
		String = String.RightChop(1);

		FString AfterAllowed;
		if (!String.Split("AllowedClasses=", nullptr, &AfterAllowed))
		{
			return;
		}

		FString AllowedStr;
		FString IgnoredStr;
		if (!AfterAllowed.Split("IgnoredClasses=", &AllowedStr, &IgnoredStr))
		{
			return;
		}

		AllowedStr.RemoveFromStart("(");
		AllowedStr.RemoveFromEnd("),");
		TArray<FString> AllowedListStr;
		AllowedStr.ParseIntoArray(AllowedListStr, TEXT(","), true);
		for (const auto& Str : AllowedListStr)
		{
			AllowedClasses.Add(TSoftClassPtr<>{Str});
		}

		IgnoredStr.RemoveFromStart("(");
		IgnoredStr.RemoveFromEnd(")");
		TArray<FString> IgnoredListStr;
		IgnoredStr.ParseIntoArray(IgnoredListStr, TEXT(","), true);
		for (const auto& Str : IgnoredListStr)
		{
			IgnoredClasses.Add(TSoftClassPtr<>{Str});
		}
	}
}

bool FClassFilter::operator==(const FClassFilter& Other) const
{
	// Do all classes match?
	return AllowedClasses.Difference(Other.AllowedClasses).Num() <= 0 &&
		IgnoredClasses.Difference(Other.IgnoredClasses).Num() <= 0;
}
