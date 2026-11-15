#pragma once

#include "CoreMinimal.h"
#include "ThesisTypes.h"

namespace ThesisText
{
	FString BehaviorLabel(const FString& BehaviorId);
	EThesisBehaviorGroup BehaviorGroup(const FString& BehaviorId);
	FString KindLabel(EThesisEnemyKind Kind);
	FString Number(double Value, int32 Decimals);
	FString Seconds(double Value, int32 Decimals = 1);
	FString Meters(double Centimeters);
	FString Clock(double Seconds);
}
