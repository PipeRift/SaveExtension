// Copyright 2015-2020 Piperift. All Rights Reserved.

#include "Misc/ClassFilter.h"

#include <Misc/Parse.h>
#include <UObject/UObjectIterator.h>


FClassFilter::FClassFilter(UClass* BaseClass)
	: BaseClass{ BaseClass }
	, IgnoredClasses {}
{}

void FClassFilter::Merge(const FClassFilter& Other)
{
	// Remove conflicts
	for (const auto& IgnoredClass : Other.IgnoredClasses)
	{
		AllowedClasses.Remove(IgnoredClass);
	}
	for (const auto& AllowedClass : Other.AllowedClasses)
	{
		IgnoredClasses.Remove(AllowedClass);
	}

	AllowedClasses.Append(Other.AllowedClasses);
	IgnoredClasses.Append(Other.IgnoredClasses);
}

void FClassFilter::BakeAllowedClasses() const
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
