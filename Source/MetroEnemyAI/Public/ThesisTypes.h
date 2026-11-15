#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"

enum class EThesisGameState : uint8
{
	MainMenu,
	Controls,
	Playing,
	Paused,
	Won,
	Lost
};

enum class EThesisLabelMode : uint8
{
	All,
	VisibleOnly,
	Off
};

enum class EThesisEnemyKind : uint8
{
	Melee,
	Ranged,
	Zombie,
	Other
};

enum class EThesisBehaviorGroup : uint8
{
	Neutral,
	Approach,
	Attack,
	Evade,
	Ally,
	Dead,
	Unknown
};

struct FThesisEnemy
{
	TWeakObjectPtr<APawn> Pawn;
	TSubclassOf<APawn> PawnClass;
	FString ClassName;
	FString DisplayName;
	EThesisEnemyKind Kind = EThesisEnemyKind::Other;
	bool bHostile = true;
	bool bSpawnedByStressTest = false;

	double MaxHealth = 100.0;
	double Health = 100.0;
	bool bAlive = true;
	double DiedAt = -1.0;

	FString BehaviorId;
	double BehaviorSince = 0.0;
	TMap<FString, double> TimeInBehavior;
	int32 Transitions = 0;
	int32 Oscillations = 0;
	FString LastTransitionFrom;
	FString LastTransitionTo;
	double LastTransitionAt = -1000.0;

	bool bTargetsPlayer = false;
	double PlayerInSightSince = -1.0;
	TArray<double> ReactionTimes;
	int32 DetectionsWithoutSight = 0;

	float SightRadius = -1.f;
	float SightHalfAngle = -1.f;
	float HearingRange = -1.f;

	double LastDistanceToPlayer = 0.0;
	double HitFlashUntil = 0.0;
};

struct FThesisLogLine
{
	double Time = 0.0;
	FString Text;
	EThesisBehaviorGroup Group = EThesisBehaviorGroup::Unknown;
};

struct FThesisToast
{
	FString Text;
	double ShownAt = 0.0;
	FLinearColor Color = FLinearColor::White;
};
