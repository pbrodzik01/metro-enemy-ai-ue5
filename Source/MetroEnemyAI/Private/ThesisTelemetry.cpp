#include "ThesisTelemetry.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "ThesisText.h"

namespace
{
	const TCHAR* Separator = TEXT(";");

	FString Join(const TArray<FString>& Cells)
	{
		return FString::Join(Cells, Separator);
	}

	FString OptionalNumber(double Value, int32 Decimals)
	{
		return Value < 0.0 ? FString() : ThesisText::Number(Value, Decimals);
	}

	bool WriteFile(const FString& Path, const TArray<FString>& Lines)
	{
		return FFileHelper::SaveStringToFile(FString::Join(Lines, TEXT("\r\n")) + TEXT("\r\n"), *Path, FFileHelper::EEncodingOptions::ForceUTF8);
	}
}

void FThesisTelemetry::Begin(const FString& InMapName)
{
	SessionStamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H-%M-%S"));
	MapName = InMapName;
	EventRows.Reset();
	PerformanceRows.Reset();
	bDirty = false;
	LastSaveFolder.Reset();
}

void FThesisTelemetry::AddEvent(double Time, const FString& EventName, const FThesisEnemy* Enemy, const FString& From, const FString& To, double PlayerHealth, int32 AliveEnemies, float Fps, double ReactionTime, const FString& Detail)
{
	if (!IsActive())
	{
		return;
	}

	EventRows.Add(Join({
		ThesisText::Number(Time, 3),
		EventName,
		Enemy ? Enemy->DisplayName : FString(),
		Enemy ? ThesisText::KindLabel(Enemy->Kind) : FString(),
		From,
		To,
		Enemy ? ThesisText::Number(Enemy->LastDistanceToPlayer / 100.0, 2) : FString(),
		Enemy ? ThesisText::Number(Enemy->Health, 0) : FString(),
		ThesisText::Number(PlayerHealth, 0),
		FString::FromInt(AliveEnemies),
		ThesisText::Number(Fps, 1),
		OptionalNumber(ReactionTime, 3),
		Detail
	}));
	bDirty = true;
}

void FThesisTelemetry::AddPerformanceSample(double Time, int32 AliveEnemies, float Fps, float FrameMilliseconds)
{
	if (!IsActive())
	{
		return;
	}

	PerformanceRows.Add(Join({
		ThesisText::Number(Time, 1),
		FString::FromInt(AliveEnemies),
		ThesisText::Number(Fps, 1),
		ThesisText::Number(FrameMilliseconds, 2)
	}));
	bDirty = true;
}

FString FThesisTelemetry::Save(const TArray<FThesisEnemy>& Enemies, double Now, const FThesisSessionStats& Stats)
{
	if (!IsActive())
	{
		return FString();
	}

	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("TelemetriaAI") / SessionStamp);
	IFileManager::Get().MakeDirectory(*Folder, true);

	TArray<FString> Events;
	Events.Add(Join({ TEXT("czas_s"), TEXT("zdarzenie"), TEXT("przeciwnik"), TEXT("typ"), TEXT("zachowanie_z"), TEXT("zachowanie_na"), TEXT("dystans_do_gracza_m"), TEXT("hp_przeciwnika"), TEXT("hp_gracza"), TEXT("aktywni_przeciwnicy"), TEXT("fps"), TEXT("czas_reakcji_s"), TEXT("szczegoly") }));
	Events.Append(EventRows);
	WriteFile(Folder / TEXT("zdarzenia.csv"), Events);

	TArray<FString> Performance;
	Performance.Add(Join({ TEXT("czas_s"), TEXT("aktywni_przeciwnicy"), TEXT("fps"), TEXT("czas_klatki_ms") }));
	Performance.Append(PerformanceRows);
	WriteFile(Folder / TEXT("wydajnosc.csv"), Performance);

	TArray<FString> BehaviorIds;
	for (const FThesisEnemy& Enemy : Enemies)
	{
		for (const TPair<FString, double>& Pair : Enemy.TimeInBehavior)
		{
			BehaviorIds.AddUnique(Pair.Key);
		}
		if (!Enemy.BehaviorId.IsEmpty())
		{
			BehaviorIds.AddUnique(Enemy.BehaviorId);
		}
	}
	BehaviorIds.Sort();

	TArray<FString> Header = { TEXT("przeciwnik"), TEXT("klasa"), TEXT("typ"), TEXT("wrogi_graczowi"), TEXT("dodany_testem"), TEXT("przezyl"), TEXT("hp_koncowe"), TEXT("zasieg_wzroku_m"), TEXT("polowa_kata_widzenia_st"), TEXT("zasieg_sluchu_m"), TEXT("zmiany_zachowania"), TEXT("oscylacje"), TEXT("wykrycia_wzrokiem"), TEXT("sr_czas_reakcji_s"), TEXT("min_czas_reakcji_s"), TEXT("max_czas_reakcji_s"), TEXT("wykrycia_bez_kontaktu_wzrokowego") };
	for (const FString& BehaviorId : BehaviorIds)
	{
		Header.Add(TEXT("czas_") + BehaviorId + TEXT("_s"));
	}

	TArray<FString> EnemyLines;
	EnemyLines.Add(Join(Header));
	for (const FThesisEnemy& Enemy : Enemies)
	{
		double Sum = 0.0;
		double Min = TNumericLimits<double>::Max();
		double Max = 0.0;
		for (const double Reaction : Enemy.ReactionTimes)
		{
			Sum += Reaction;
			Min = FMath::Min(Min, Reaction);
			Max = FMath::Max(Max, Reaction);
		}
		const bool bHasReactions = Enemy.ReactionTimes.Num() > 0;

		TArray<FString> Row = {
			Enemy.DisplayName,
			Enemy.ClassName,
			ThesisText::KindLabel(Enemy.Kind),
			Enemy.bHostile ? TEXT("tak") : TEXT("nie"),
			Enemy.bSpawnedByStressTest ? TEXT("tak") : TEXT("nie"),
			Enemy.bAlive ? TEXT("tak") : TEXT("nie"),
			ThesisText::Number(Enemy.Health, 0),
			Enemy.SightRadius > 0.f ? ThesisText::Number(Enemy.SightRadius / 100.0, 1) : FString(),
			Enemy.SightHalfAngle > 0.f ? ThesisText::Number(Enemy.SightHalfAngle, 0) : FString(),
			Enemy.HearingRange > 0.f ? ThesisText::Number(Enemy.HearingRange / 100.0, 1) : FString(),
			FString::FromInt(Enemy.Transitions),
			FString::FromInt(Enemy.Oscillations),
			FString::FromInt(Enemy.ReactionTimes.Num()),
			bHasReactions ? ThesisText::Number(Sum / Enemy.ReactionTimes.Num(), 3) : FString(),
			bHasReactions ? ThesisText::Number(Min, 3) : FString(),
			bHasReactions ? ThesisText::Number(Max, 3) : FString(),
			FString::FromInt(Enemy.DetectionsWithoutSight)
		};

		for (const FString& BehaviorId : BehaviorIds)
		{
			double Seconds = Enemy.TimeInBehavior.FindRef(BehaviorId);
			if (Enemy.bAlive && Enemy.BehaviorId == BehaviorId)
			{
				Seconds += FMath::Max(0.0, Now - Enemy.BehaviorSince);
			}
			Row.Add(ThesisText::Number(Seconds, 2));
		}
		EnemyLines.Add(Join(Row));
	}
	WriteFile(Folder / TEXT("przeciwnicy.csv"), EnemyLines);

	TArray<FString> Session = {
		Join({ TEXT("parametr"), TEXT("wartosc") }),
		Join({ TEXT("mapa"), Stats.MapName }),
		Join({ TEXT("wynik"), Stats.Outcome }),
		Join({ TEXT("czas_sesji_s"), ThesisText::Number(Stats.SessionTime, 1) }),
		Join({ TEXT("wrogowie_lacznie"), FString::FromInt(Stats.HostileTotal) }),
		Join({ TEXT("eliminacje"), FString::FromInt(Stats.Kills) }),
		Join({ TEXT("dodani_testem_obciazenia"), FString::FromInt(Stats.StressSpawned) }),
		Join({ TEXT("maks_aktywnych_przeciwnikow"), FString::FromInt(Stats.PeakAliveEnemies) }),
		Join({ TEXT("sredni_fps"), ThesisText::Number(Stats.AverageFps, 1) }),
		Join({ TEXT("minimalny_fps"), ThesisText::Number(Stats.MinimumFps, 1) }),
		Join({ TEXT("liczba_zdarzen"), FString::FromInt(EventRows.Num()) })
	};
	WriteFile(Folder / TEXT("sesja.csv"), Session);

	bDirty = false;
	LastSaveFolder = Folder;
	return Folder;
}
