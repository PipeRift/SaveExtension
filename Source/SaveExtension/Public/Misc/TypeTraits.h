// Copyright 2015-2019 Piperift. All Rights Reserved.

#pragma once

#include "Templates/UnrealTypeTraits.h"


template <typename Type>
constexpr bool VariadicContainsType() {
	return false;
};

template <typename Type, typename Other, typename... T>
constexpr bool VariadicContainsType() {
	return TIsSame<Type, Other>::Value || VariadicContainsType<Type, T...>();
};


template <uint32 Index, typename Type>
constexpr uint32 GetVariadicTypeIndex() {
	return Index+1;
};

template <uint32 Index, typename Type, typename Other, typename... T>
constexpr uint32 GetVariadicTypeIndex() {
	if (TIsSame<Type, Other>::Value)
		return Index;
	else
		return GetVariadicTypeIndex<Index + 1, Type, T...>();
};
