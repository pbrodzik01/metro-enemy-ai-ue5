#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "ThesisTypes.h"

namespace ThesisStyle
{
	FLinearColor PanelColor();
	FLinearColor PanelStrongColor();
	FLinearColor TextColor();
	FLinearColor TextDimColor();
	FLinearColor AccentColor();
	FLinearColor DangerColor();
	FLinearColor GoodColor();
	FLinearColor ShadowColor();
	FLinearColor BehaviorColor(EThesisBehaviorGroup Group);
	FLinearColor HealthColor(double Fraction);

	const FSlateBrush* Solid();
	const FSlateBrush* Rounded();
	const FSlateBrush* RoundedSmall();
	const FSlateBrush* Pill();

	FSlateFontInfo Font(int32 Size, bool bBold = false);
	const FButtonStyle& Button(bool bPrimary);
}
