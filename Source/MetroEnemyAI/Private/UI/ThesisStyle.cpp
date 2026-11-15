#include "UI/ThesisStyle.h"

#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"

namespace ThesisStyle
{
	namespace
	{
		FLinearColor Hex(const TCHAR* Value, float Alpha = 1.f)
		{
			FLinearColor Color = FLinearColor(FColor::FromHex(Value));
			Color.A = Alpha;
			return Color;
		}

		FButtonStyle MakeButtonStyle(const FLinearColor& Normal, const FLinearColor& Hovered, const FLinearColor& Pressed)
		{
			return FButtonStyle()
				.SetNormal(FSlateRoundedBoxBrush(Normal, 6.f))
				.SetHovered(FSlateRoundedBoxBrush(Hovered, 6.f))
				.SetPressed(FSlateRoundedBoxBrush(Pressed, 6.f))
				.SetDisabled(FSlateRoundedBoxBrush(Hex(TEXT("2A2F38"), 0.6f), 6.f))
				.SetNormalPadding(FMargin(0.f))
				.SetPressedPadding(FMargin(0.f, 1.f, 0.f, 0.f));
		}
	}

	FLinearColor PanelColor() { return Hex(TEXT("0E1116"), 0.82f); }
	FLinearColor PanelStrongColor() { return Hex(TEXT("12161D"), 0.96f); }
	FLinearColor TextColor() { return Hex(TEXT("EEF1F5")); }
	FLinearColor TextDimColor() { return Hex(TEXT("9AA3AF")); }
	FLinearColor AccentColor() { return Hex(TEXT("F2A93B")); }
	FLinearColor DangerColor() { return Hex(TEXT("E5484D")); }
	FLinearColor GoodColor() { return Hex(TEXT("46C27A")); }
	FLinearColor ShadowColor() { return FLinearColor(0.f, 0.f, 0.f, 0.65f); }

	FLinearColor BehaviorColor(EThesisBehaviorGroup Group)
	{
		switch (Group)
		{
		case EThesisBehaviorGroup::Neutral: return Hex(TEXT("2F7EA1"));
		case EThesisBehaviorGroup::Approach: return Hex(TEXT("D98A1C"));
		case EThesisBehaviorGroup::Attack: return Hex(TEXT("C93434"));
		case EThesisBehaviorGroup::Evade: return Hex(TEXT("8A5CC7"));
		case EThesisBehaviorGroup::Ally: return Hex(TEXT("2F9E5C"));
		case EThesisBehaviorGroup::Dead: return Hex(TEXT("4A505A"));
		default: return Hex(TEXT("5B6370"));
		}
	}

	FLinearColor HealthColor(double Fraction)
	{
		const FLinearColor Low = DangerColor();
		const FLinearColor Mid = AccentColor();
		const FLinearColor High = GoodColor();
		const float Clamped = static_cast<float>(FMath::Clamp(Fraction, 0.0, 1.0));
		return Clamped < 0.5f
			? FLinearColor::LerpUsingHSV(Low, Mid, Clamped * 2.f)
			: FLinearColor::LerpUsingHSV(Mid, High, (Clamped - 0.5f) * 2.f);
	}

	const FSlateBrush* Solid()
	{
		static const FSlateColorBrush Brush(FLinearColor::White);
		return &Brush;
	}

	const FSlateBrush* Rounded()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White, 8.f);
		return &Brush;
	}

	const FSlateBrush* RoundedSmall()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White, 3.f);
		return &Brush;
	}

	const FSlateBrush* Pill()
	{
		static const FSlateRoundedBoxBrush Brush(FLinearColor::White, 11.f);
		return &Brush;
	}

	FSlateFontInfo Font(int32 Size, bool bBold)
	{
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", Size);
	}

	const FButtonStyle& Button(bool bPrimary)
	{
		static const FButtonStyle Primary = MakeButtonStyle(Hex(TEXT("E39A2D")), Hex(TEXT("F5B451")), Hex(TEXT("C27F1C")));
		static const FButtonStyle Secondary = MakeButtonStyle(Hex(TEXT("232933")), Hex(TEXT("2F3744")), Hex(TEXT("1B2028")));
		return bPrimary ? Primary : Secondary;
	}
}
