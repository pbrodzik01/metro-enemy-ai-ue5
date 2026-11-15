#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ThesisTypes.h"
#include "ThesisTelemetry.h"
#include "ThesisGameSubsystem.generated.h"

class SThesisOverlay;
class UGameViewportClient;
class UThesisSessionSubsystem;
class APlayerController;
class UBlackboardComponent;

UCLASS()
class METROENEMYAI_API UThesisGameSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableWhenPaused() const override { return true; }

	EThesisGameState GetGameState() const { return GameState; }
	const TArray<FThesisEnemy>& GetEnemies() const { return Enemies; }
	const TArray<FThesisLogLine>& GetDecisionLog() const { return DecisionLog; }
	const TArray<FThesisToast>& GetToasts() const { return Toasts; }

	double GetNow() const;
	double GetRealTime() const;
	double GetSessionTime() const;
	bool HasSessionStarted() const { return SessionStart >= 0.0; }
	bool IsAutomationActive() const { return bTourMode || BenchmarkSeconds > 0.0; }
	double GetPlayerHealth() const { return PlayerHealth; }
	double GetPlayerMaxHealth() const { return PlayerMaxHealth; }
	bool IsPlayerAlive() const { return bPlayerAlive; }
	int32 GetHostileTotal() const;
	int32 GetHostileAlive() const;
	int32 GetKills() const { return Kills; }
	int32 GetStressSpawned() const { return StressSpawned; }
	float GetFps() const { return CurrentFps; }
	int32 GetTotalTransitions() const;
	int32 GetTotalOscillations() const;
	double GetAverageReaction() const;
	double GetDamageFlashUntil() const { return DamageFlashUntil; }
	double GetHitMarkerUntil() const { return HitMarkerUntil; }
	EThesisLabelMode GetLabelMode() const;
	bool IsDebugPanelVisible() const;
	const FString& GetTelemetryFolder() const { return Telemetry.GetLastSaveFolder(); }
	APlayerController* GetPlayerController() const;
	APawn* GetPlayerPawn() const;

	void StartGame();
	void ShowControls();
	void CloseControls();
	void TogglePause();
	void Resume();
	void RestartLevel(bool bShowMainMenu);
	void QuitGame();
	void ToggleDebugPanel();
	void CycleLabelMode();
	void SpawnStressWave();
	void SaveTelemetry(bool bAnnounce);

protected:
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

private:
	void EnsureOverlay();
	void RemoveOverlay();
	void ApplyMenuMode(bool bMenuOpen);
	void SetGameState(EThesisGameState NewState);
	void PollKeys();
	void UpdateFrameStats();
	void HideLegacyHealthBar();
	void DiscoverEnemies();
	void UpdatePlayer();
	void UpdateEnemies();
	void UpdateEnemy(FThesisEnemy& Enemy);
	void OnBehaviorChanged(FThesisEnemy& Enemy, const FString& NewBehavior);
	void OnTargetChanged(FThesisEnemy& Enemy, bool bNowTargetsPlayer);
	void MarkEnemyDead(FThesisEnemy& Enemy, const FString& Reason);
	void UpdatePlayerSightTimer(FThesisEnemy& Enemy, APawn* EnemyPawn);
	bool IsPlayerInSightCone(const FThesisEnemy& Enemy, APawn* EnemyPawn) const;
	void ReadPerceptionConfig(FThesisEnemy& Enemy, APawn* EnemyPawn) const;
	void LogEnemyTags(const FThesisEnemy& Enemy, APawn* EnemyPawn) const;
	bool IsHostileToPlayer(APawn* EnemyPawn) const;
	FString ReadBehavior(UBlackboardComponent* Blackboard) const;
	void CheckEndConditions();
	void FinishGame(bool bWon);
	void UpdateAutomation();
	void UpdateTour(double Elapsed);
	void UpdateBenchmark();
	void CaptureScreenshot(const FString& Name) const;
	void MoveToTourSpot();
	void AddLog(const FString& Text, EThesisBehaviorGroup Group);
	void AddToast(const FString& Text, const FLinearColor& Color);
	void AddTelemetryEvent(const FString& EventName, const FThesisEnemy* Enemy, const FString& From, const FString& To, double ReactionTime, const FString& Detail);
	FThesisSessionStats BuildSessionStats(const FString& Outcome) const;
	UThesisSessionSubsystem* GetSession() const;

	TSharedPtr<SThesisOverlay> Overlay;
	TWeakObjectPtr<UGameViewportClient> OverlayViewport;

	EThesisGameState GameState = EThesisGameState::MainMenu;
	EThesisGameState ControlsReturnState = EThesisGameState::MainMenu;
	bool bInitialStateResolved = false;

	TArray<FThesisEnemy> Enemies;
	TArray<FThesisLogLine> DecisionLog;
	TArray<FThesisToast> Toasts;
	TMap<EThesisEnemyKind, int32> KindCounters;

	FThesisTelemetry Telemetry;
	double SessionStart = -1.0;
	double SessionEnd = -1.0;
	double LastDiscovery = -1000.0;
	double LastLegacyHudCheck = -1000.0;
	double LastPerformanceSample = -1000.0;
	double EndScreenAt = -1.0;
	bool bPendingWin = false;

	double PlayerHealth = 100.0;
	double PlayerMaxHealth = 100.0;
	bool bPlayerAlive = true;
	bool bPlayerSeen = false;

	int32 Kills = 0;
	int32 StressSpawned = 0;
	int32 PeakAlive = 0;

	int32 FrameCount = 0;
	double FrameTimeAccumulator = 0.0;
	float CurrentFps = 0.f;
	float CurrentFrameMs = 0.f;
	double FpsSum = 0.0;
	int32 FpsSamples = 0;
	float MinFps = 0.f;

	double DamageFlashUntil = 0.0;
	double HitMarkerUntil = 0.0;

	bool bTourMode = false;
	bool bGodMode = false;
	double BenchmarkSeconds = 0.0;
	double AutomationStart = -1.0;
	double LastBenchmarkWave = -1.0;
	int32 TourStep = 0;
	double LastTourStepTime = 0.0;
	uint64 LastTourStepFrame = 0;
	FString AutomationOutcome;
};
