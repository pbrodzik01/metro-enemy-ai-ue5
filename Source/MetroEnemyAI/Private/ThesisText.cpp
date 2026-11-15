#include "ThesisText.h"

#include "Internationalization/Internationalization.h"

namespace ThesisText
{
	namespace
	{
		FString Normalize(const FString& BehaviorId)
		{
			return BehaviorId.Replace(TEXT(" "), TEXT("")).ToLower();
		}
	}

	FString BehaviorLabel(const FString& BehaviorId)
	{
		const FString Key = Normalize(BehaviorId);
		if (Key == TEXT("idle")) return TEXT("Bezczynność");
		if (Key == TEXT("patrol")) return TEXT("Patrol");
		if (Key == TEXT("roaming")) return TEXT("Wędrówka");
		if (Key == TEXT("approach")) return TEXT("Pościg");
		if (Key == TEXT("meleeattack")) return TEXT("Atak wręcz");
		if (Key == TEXT("rangeattack")) return TEXT("Atak z dystansu");
		if (Key == TEXT("strafe")) return TEXT("Ruch boczny");
		if (Key == TEXT("companion")) return TEXT("Towarzysz");
		if (Key == TEXT("dead")) return TEXT("Wyeliminowany");
		if (Key.IsEmpty()) return TEXT("Brak danych");
		return BehaviorId;
	}

	EThesisBehaviorGroup BehaviorGroup(const FString& BehaviorId)
	{
		const FString Key = Normalize(BehaviorId);
		if (Key == TEXT("idle") || Key == TEXT("patrol") || Key == TEXT("roaming")) return EThesisBehaviorGroup::Neutral;
		if (Key == TEXT("approach")) return EThesisBehaviorGroup::Approach;
		if (Key == TEXT("meleeattack") || Key == TEXT("rangeattack")) return EThesisBehaviorGroup::Attack;
		if (Key == TEXT("strafe")) return EThesisBehaviorGroup::Evade;
		if (Key == TEXT("companion")) return EThesisBehaviorGroup::Ally;
		if (Key == TEXT("dead")) return EThesisBehaviorGroup::Dead;
		return EThesisBehaviorGroup::Unknown;
	}

	FString KindLabel(EThesisEnemyKind Kind)
	{
		switch (Kind)
		{
		case EThesisEnemyKind::Melee: return TEXT("Kontaktowy");
		case EThesisEnemyKind::Ranged: return TEXT("Dystansowy");
		case EThesisEnemyKind::Zombie: return TEXT("Zombie");
		default: return TEXT("Przeciwnik");
		}
	}

	FString Number(double Value, int32 Decimals)
	{
		FNumberFormattingOptions Options;
		Options.SetUseGrouping(false);
		Options.SetMinimumFractionalDigits(Decimals);
		Options.SetMaximumFractionalDigits(Decimals);
		return FText::AsNumber(Value, &Options, FInternationalization::Get().GetInvariantCulture()).ToString().Replace(TEXT("."), TEXT(","));
	}

	FString Seconds(double Value, int32 Decimals)
	{
		return Number(Value, Decimals) + TEXT(" s");
	}

	FString Meters(double Centimeters)
	{
		return Number(Centimeters / 100.0, 1) + TEXT(" m");
	}

	FString Clock(double Seconds)
	{
		const int32 Total = FMath::Max(0, FMath::FloorToInt(Seconds));
		return FString::Printf(TEXT("%02d:%02d"), Total / 60, Total % 60);
	}
}
