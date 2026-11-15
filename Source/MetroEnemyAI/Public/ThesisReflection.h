#pragma once

#include "CoreMinimal.h"

namespace ThesisReflection
{
	bool GetNumber(const UObject* Object, FName PropertyName, double& OutValue);
	bool SetNumber(UObject* Object, FName PropertyName, double Value);
	bool GetNameArray(const UObject* Object, FName PropertyName, TArray<FName>& OutNames);
	bool CallBoolFunction(UObject* Object, FName FunctionName, bool& OutValue);
}
