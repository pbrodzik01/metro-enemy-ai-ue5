#pragma once

#include "CoreMinimal.h"
#include "ThesisTypes.h"

struct FThesisSessionStats
{
	FString MapName;
	FString Outcome;
	double SessionTime = 0.0;
	int32 Kills = 0;
	int32 HostileTotal = 0;
	int32 StressSpawned = 0;
	float AverageFps = 0.f;
	float MinimumFps = 0.f;
	int32 PeakAliveEnemies = 0;
};

class FThesisTelemetry
{
public:
	void Begin(const FString& InMapName);
	bool IsActive() const { return !SessionStamp.IsEmpty(); }

	void AddEvent(double Time, const FString& EventName, const FThesisEnemy* Enemy, const FString& From, const FString& To, double PlayerHealth, int32 AliveEnemies, float Fps, double ReactionTime, const FString& Detail);
	void AddPerformanceSample(double Time, int32 AliveEnemies, float Fps, float FrameMilliseconds);

	FString Save(const TArray<FThesisEnemy>& Enemies, double Now, const FThesisSessionStats& Stats);

	bool HasUnsavedData() const { return bDirty; }
	const FString& GetLastSaveFolder() const { return LastSaveFolder; }
	int32 GetEventCount() const { return EventRows.Num(); }

private:
	FString SessionStamp;
	FString MapName;
	TArray<FString> EventRows;
	TArray<FString> PerformanceRows;
	bool bDirty = false;
	FString LastSaveFolder;
};
