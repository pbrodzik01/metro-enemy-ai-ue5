#include "ThesisGameSubsystem.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Widget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/App.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "NavigationSystem.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "ThesisReflection.h"
#include "ThesisSessionSubsystem.h"
#include "ThesisText.h"
#include "UI/SThesisOverlay.h"
#include "UI/ThesisStyle.h"

DEFINE_LOG_CATEGORY_STATIC(LogThesisUI, Log, All);

namespace
{
	TAutoConsoleVariable<int32> CVarThesisSkipMenu(
		TEXT("thesis.SkipMenu"),
		0,
		TEXT("1 = start the level without the thesis main menu."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarThesisTour(
		TEXT("thesis.Tour"),
		0,
		TEXT("1 = walk through every UI screen automatically, save screenshots to Saved/Screenshots/ThesisTour and quit."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarThesisBenchmark(
		TEXT("thesis.Benchmark"),
		0,
		TEXT("N > 0 = automatic measurement session of N seconds: invulnerable player, stress waves, telemetry saved on quit."),
		ECVF_Default);

	TAutoConsoleVariable<float> CVarThesisBenchmarkWave(
		TEXT("thesis.BenchmarkWave"),
		20.f,
		TEXT("Seconds between stress waves in benchmark mode."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarThesisBenchmarkMax(
		TEXT("thesis.BenchmarkMax"),
		40,
		TEXT("Benchmark mode stops adding waves above this number of living enemies."),
		ECVF_Default);

	TAutoConsoleVariable<int32> CVarThesisGodMode(
		TEXT("thesis.GodMode"),
		0,
		TEXT("1 = keep the player's CurrentHealth at maximum."),
		ECVF_Default);

	const FName HealthProperty(TEXT("CurrentHealth"));
	const FName TargetTagsProperty(TEXT("TargetTags"));
	const FName IsAliveFunction(TEXT("IsAlive?"));
	const FName BehaviorKey(TEXT("Behavior"));
	const FName TargetKey(TEXT("Target"));
	const FName LegacyHealthBarName(TEXT("Healthbar"));

	constexpr double DiscoveryInterval = 0.5;
	constexpr double OscillationWindow = 1.0;
	constexpr double ToastLifetime = 3.5;
	constexpr int32 MaxLogLines = 12;
	constexpr int32 MaxToasts = 4;
	constexpr int32 StressWaveSize = 5;

	EThesisEnemyKind KindFromClass(const UClass* Class)
	{
		const FString Name = Class ? Class->GetName() : FString();
		if (Name.Contains(TEXT("Range"))) return EThesisEnemyKind::Ranged;
		if (Name.Contains(TEXT("Melee"))) return EThesisEnemyKind::Melee;
		if (Name.Contains(TEXT("Zombie"))) return EThesisEnemyKind::Zombie;
		return EThesisEnemyKind::Other;
	}
}

bool UThesisGameSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UThesisGameSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UThesisGameSubsystem, STATGROUP_Tickables);
}

void UThesisGameSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	GameState = EThesisGameState::MainMenu;
	bInitialStateResolved = false;
}

void UThesisGameSubsystem::Deinitialize()
{
	if (Telemetry.IsActive() && Telemetry.HasUnsavedData())
	{
		Telemetry.Save(Enemies, GetNow(), BuildSessionStats(TEXT("przerwana")));
	}
	RemoveOverlay();
	Super::Deinitialize();
}

double UThesisGameSubsystem::GetNow() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() : 0.0;
}

double UThesisGameSubsystem::GetRealTime() const
{
	return FPlatformTime::Seconds();
}

double UThesisGameSubsystem::GetSessionTime() const
{
	if (SessionStart < 0.0)
	{
		return 0.0;
	}
	return (SessionEnd >= 0.0 ? SessionEnd : GetNow()) - SessionStart;
}

UThesisSessionSubsystem* UThesisGameSubsystem::GetSession() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UThesisSessionSubsystem>() : nullptr;
}

EThesisLabelMode UThesisGameSubsystem::GetLabelMode() const
{
	const UThesisSessionSubsystem* Session = GetSession();
	return Session ? Session->LabelMode : EThesisLabelMode::All;
}

bool UThesisGameSubsystem::IsDebugPanelVisible() const
{
	const UThesisSessionSubsystem* Session = GetSession();
	return Session && Session->bDebugPanelVisible;
}

APlayerController* UThesisGameSubsystem::GetPlayerController() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetFirstPlayerController() : nullptr;
}

APawn* UThesisGameSubsystem::GetPlayerPawn() const
{
	const APlayerController* Controller = GetPlayerController();
	return Controller ? Controller->GetPawn() : nullptr;
}

int32 UThesisGameSubsystem::GetHostileTotal() const
{
	int32 Count = 0;
	for (const FThesisEnemy& Enemy : Enemies)
	{
		Count += Enemy.bHostile ? 1 : 0;
	}
	return Count;
}

int32 UThesisGameSubsystem::GetHostileAlive() const
{
	int32 Count = 0;
	for (const FThesisEnemy& Enemy : Enemies)
	{
		Count += (Enemy.bHostile && Enemy.bAlive) ? 1 : 0;
	}
	return Count;
}

int32 UThesisGameSubsystem::GetTotalTransitions() const
{
	int32 Count = 0;
	for (const FThesisEnemy& Enemy : Enemies)
	{
		Count += Enemy.Transitions;
	}
	return Count;
}

int32 UThesisGameSubsystem::GetTotalOscillations() const
{
	int32 Count = 0;
	for (const FThesisEnemy& Enemy : Enemies)
	{
		Count += Enemy.Oscillations;
	}
	return Count;
}

double UThesisGameSubsystem::GetAverageReaction() const
{
	double Sum = 0.0;
	int32 Count = 0;
	for (const FThesisEnemy& Enemy : Enemies)
	{
		for (const double Reaction : Enemy.ReactionTimes)
		{
			Sum += Reaction;
			++Count;
		}
	}
	return Count > 0 ? Sum / Count : -1.0;
}

void UThesisGameSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UWorld* World = GetWorld();
	if (!World || !World->HasBegunPlay() || !GetPlayerController())
	{
		return;
	}

	EnsureOverlay();
	if (!Overlay.IsValid())
	{
		return;
	}

	if (!bInitialStateResolved)
	{
		bInitialStateResolved = true;
		if (IConsoleVariable* OverflowMessages = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shadow.Virtual.AllowScreenOverflowMessages")))
		{
			OverflowMessages->Set(0, ECVF_SetByCode);
		}
		int32 CommandLineBenchmark = 0;
		FParse::Value(FCommandLine::Get(), TEXT("ThesisBenchmark="), CommandLineBenchmark);
		bTourMode = CVarThesisTour.GetValueOnGameThread() != 0 || FParse::Param(FCommandLine::Get(), TEXT("ThesisTour"));
		BenchmarkSeconds = bTourMode ? 0.0 : static_cast<double>(FMath::Max(CVarThesisBenchmark.GetValueOnGameThread(), CommandLineBenchmark));
		bGodMode = IsAutomationActive() || CVarThesisGodMode.GetValueOnGameThread() != 0;
		AutomationStart = GetRealTime();

		UThesisSessionSubsystem* Session = GetSession();
		const bool bSkipMenu = (Session && Session->ConsumeSkipMenu()) || CVarThesisSkipMenu.GetValueOnGameThread() != 0;
		if (BenchmarkSeconds > 0.0)
		{
			if (Session)
			{
				Session->bDebugPanelVisible = true;
			}
			StartGame();
			LastBenchmarkWave = GetNow();
		}
		else if (bSkipMenu && !bTourMode)
		{
			StartGame();
		}
		else
		{
			SetGameState(EThesisGameState::MainMenu);
		}
	}

	UpdateFrameStats();
	PollKeys();
	UpdateAutomation();

	if (GetRealTime() - LastLegacyHudCheck > 0.5)
	{
		LastLegacyHudCheck = GetRealTime();
		HideLegacyHealthBar();
	}

	if (GameState == EThesisGameState::Playing)
	{
		if (GetNow() - LastDiscovery > DiscoveryInterval)
		{
			LastDiscovery = GetNow();
			DiscoverEnemies();
		}
		UpdatePlayer();
		UpdateEnemies();
		CheckEndConditions();

		if (GetNow() - LastPerformanceSample >= 1.0 && CurrentFps > 0.f)
		{
			LastPerformanceSample = GetNow();
			int32 AliveCount = 0;
			for (const FThesisEnemy& Enemy : Enemies)
			{
				AliveCount += Enemy.bAlive ? 1 : 0;
			}
			PeakAlive = FMath::Max(PeakAlive, AliveCount);
			Telemetry.AddPerformanceSample(GetSessionTime(), AliveCount, CurrentFps, CurrentFrameMs);
			FpsSum += CurrentFps;
			++FpsSamples;
			MinFps = FpsSamples == 1 ? CurrentFps : FMath::Min(MinFps, CurrentFps);
		}
	}

	const double Real = GetRealTime();
	Toasts.RemoveAll([Real](const FThesisToast& Toast) { return Real - Toast.ShownAt > ToastLifetime; });
}

void UThesisGameSubsystem::UpdateFrameStats()
{
	++FrameCount;
	FrameTimeAccumulator += FApp::GetDeltaTime();
	if (FrameTimeAccumulator >= 1.0)
	{
		CurrentFps = static_cast<float>(FrameCount / FrameTimeAccumulator);
		CurrentFrameMs = static_cast<float>(1000.0 * FrameTimeAccumulator / FrameCount);
		FrameCount = 0;
		FrameTimeAccumulator = 0.0;
	}
}

void UThesisGameSubsystem::EnsureOverlay()
{
	UWorld* World = GetWorld();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	if (!Viewport || (Overlay.IsValid() && OverlayViewport.Get() == Viewport))
	{
		return;
	}

	RemoveOverlay();
	Overlay = SNew(SThesisOverlay).Subsystem(this);
	Viewport->AddViewportWidgetContent(Overlay.ToSharedRef(), 100);
	OverlayViewport = Viewport;
}

void UThesisGameSubsystem::RemoveOverlay()
{
	if (Overlay.IsValid())
	{
		if (UGameViewportClient* Viewport = OverlayViewport.Get())
		{
			Viewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
		}
	}
	Overlay.Reset();
	OverlayViewport.Reset();
}

void UThesisGameSubsystem::SetGameState(EThesisGameState NewState)
{
	GameState = NewState;
	ApplyMenuMode(NewState != EThesisGameState::Playing);
}

void UThesisGameSubsystem::ApplyMenuMode(bool bMenuOpen)
{
	APlayerController* Controller = GetPlayerController();
	if (!Controller)
	{
		return;
	}

	if (IsAutomationActive())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		Controller->SetInputMode(InputMode);
		Controller->SetShowMouseCursor(true);
	}
	else if (bMenuOpen)
	{
		FInputModeUIOnly InputMode;
		if (Overlay.IsValid())
		{
			InputMode.SetWidgetToFocus(Overlay->GetMenuFocusTarget());
		}
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Controller->SetInputMode(InputMode);
		Controller->SetShowMouseCursor(true);
	}
	else
	{
		Controller->SetInputMode(FInputModeGameOnly());
		Controller->SetShowMouseCursor(false);
		Controller->FlushPressedKeys();
	}

	UGameplayStatics::SetGamePaused(GetWorld(), bMenuOpen);
}

void UThesisGameSubsystem::PollKeys()
{
	APlayerController* Controller = GetPlayerController();
	if (!Controller || GameState != EThesisGameState::Playing)
	{
		return;
	}

	if (Controller->WasInputKeyJustPressed(EKeys::P) || Controller->WasInputKeyJustPressed(EKeys::Escape))
	{
		TogglePause();
		return;
	}
	if (Controller->WasInputKeyJustPressed(EKeys::F1))
	{
		ToggleDebugPanel();
	}
	if (Controller->WasInputKeyJustPressed(EKeys::F2))
	{
		CycleLabelMode();
	}
	if (Controller->WasInputKeyJustPressed(EKeys::F3))
	{
		SaveTelemetry(true);
	}
	if (Controller->WasInputKeyJustPressed(EKeys::F5))
	{
		SpawnStressWave();
	}
}

void UThesisGameSubsystem::HideLegacyHealthBar()
{
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), Widgets, UUserWidget::StaticClass(), false);
	for (UUserWidget* Widget : Widgets)
	{
		if (Widget && Widget->GetClass()->GetName().StartsWith(TEXT("WB_Player")))
		{
			if (UWidget* HealthBar = Widget->GetWidgetFromName(LegacyHealthBarName))
			{
				HealthBar->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void UThesisGameSubsystem::StartGame()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (SessionStart < 0.0)
	{
		SessionStart = GetNow();
		Telemetry.Begin(UGameplayStatics::GetCurrentLevelName(World, true));
		AddTelemetryEvent(TEXT("START"), nullptr, FString(), FString(), -1.0, FString());
		LastDiscovery = -1000.0;
	}
	SetGameState(EThesisGameState::Playing);
	DiscoverEnemies();
}

void UThesisGameSubsystem::ShowControls()
{
	if (GameState == EThesisGameState::MainMenu || GameState == EThesisGameState::Paused)
	{
		ControlsReturnState = GameState;
		SetGameState(EThesisGameState::Controls);
	}
}

void UThesisGameSubsystem::CloseControls()
{
	if (GameState == EThesisGameState::Controls)
	{
		SetGameState(ControlsReturnState);
	}
}

void UThesisGameSubsystem::TogglePause()
{
	if (GameState == EThesisGameState::Playing)
	{
		SetGameState(EThesisGameState::Paused);
	}
	else if (GameState == EThesisGameState::Paused)
	{
		Resume();
	}
}

void UThesisGameSubsystem::Resume()
{
	if (GameState == EThesisGameState::Paused)
	{
		SetGameState(EThesisGameState::Playing);
	}
}

void UThesisGameSubsystem::RestartLevel(bool bShowMainMenu)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (Telemetry.IsActive() && Telemetry.HasUnsavedData())
	{
		SaveTelemetry(false);
	}
	if (!bShowMainMenu)
	{
		if (UThesisSessionSubsystem* Session = GetSession())
		{
			Session->RequestSkipMenuOnNextLoad();
		}
	}

	UGameplayStatics::SetGamePaused(World, false);
	UGameplayStatics::OpenLevel(World, FName(*UGameplayStatics::GetCurrentLevelName(World, true)));
}

void UThesisGameSubsystem::QuitGame()
{
	if (Telemetry.IsActive() && Telemetry.HasUnsavedData())
	{
		SaveTelemetry(false);
	}
	UKismetSystemLibrary::QuitGame(GetWorld(), GetPlayerController(), EQuitPreference::Quit, false);
}

void UThesisGameSubsystem::ToggleDebugPanel()
{
	if (UThesisSessionSubsystem* Session = GetSession())
	{
		Session->bDebugPanelVisible = !Session->bDebugPanelVisible;
	}
}

void UThesisGameSubsystem::CycleLabelMode()
{
	UThesisSessionSubsystem* Session = GetSession();
	if (!Session)
	{
		return;
	}

	switch (Session->LabelMode)
	{
	case EThesisLabelMode::All:
		Session->LabelMode = EThesisLabelMode::VisibleOnly;
		AddToast(TEXT("Etykiety AI: tylko widoczni przeciwnicy"), ThesisStyle::TextColor());
		break;
	case EThesisLabelMode::VisibleOnly:
		Session->LabelMode = EThesisLabelMode::Off;
		AddToast(TEXT("Etykiety AI: wyłączone"), ThesisStyle::TextColor());
		break;
	default:
		Session->LabelMode = EThesisLabelMode::All;
		AddToast(TEXT("Etykiety AI: wszyscy przeciwnicy"), ThesisStyle::TextColor());
		break;
	}
}

void UThesisGameSubsystem::SaveTelemetry(bool bAnnounce)
{
	if (!Telemetry.IsActive())
	{
		return;
	}

	FString Outcome = TEXT("w toku");
	if (GameState == EThesisGameState::Won)
	{
		Outcome = TEXT("zwyciestwo");
	}
	else if (GameState == EThesisGameState::Lost)
	{
		Outcome = TEXT("porazka");
	}
	if (!AutomationOutcome.IsEmpty())
	{
		Outcome = AutomationOutcome;
	}

	const FString Folder = Telemetry.Save(Enemies, GetNow(), BuildSessionStats(Outcome));
	UE_LOG(LogThesisUI, Log, TEXT("Telemetry saved to %s"), *Folder);
	if (bAnnounce)
	{
		AddToast(TEXT("Zapisano telemetrię: Saved/TelemetriaAI"), ThesisStyle::GoodColor());
	}
}

FThesisSessionStats UThesisGameSubsystem::BuildSessionStats(const FString& Outcome) const
{
	FThesisSessionStats Stats;
	if (const UWorld* World = GetWorld())
	{
		Stats.MapName = UGameplayStatics::GetCurrentLevelName(World, true);
	}
	Stats.Outcome = Outcome;
	Stats.SessionTime = GetSessionTime();
	Stats.Kills = Kills;
	Stats.HostileTotal = GetHostileTotal();
	Stats.StressSpawned = StressSpawned;
	Stats.AverageFps = FpsSamples > 0 ? static_cast<float>(FpsSum / FpsSamples) : 0.f;
	Stats.MinimumFps = MinFps;
	Stats.PeakAliveEnemies = PeakAlive;
	return Stats;
}

void UThesisGameSubsystem::SpawnStressWave()
{
	UWorld* World = GetWorld();
	APawn* Player = GetPlayerPawn();
	UNavigationSystemV1* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!World || !Player || !Navigation)
	{
		return;
	}

	TArray<TSubclassOf<APawn>> Classes;
	for (const FThesisEnemy& Enemy : Enemies)
	{
		if (Enemy.bHostile && Enemy.PawnClass)
		{
			Classes.AddUnique(Enemy.PawnClass);
		}
	}
	if (Classes.Num() == 0)
	{
		AddToast(TEXT("Brak klas przeciwników do testu obciążenia"), ThesisStyle::DangerColor());
		return;
	}

	int32 Spawned = 0;
	const FVector Origin = Player->GetActorLocation();
	for (int32 Index = 0; Index < StressWaveSize; ++Index)
	{
		FNavLocation Location;
		bool bFound = false;
		for (int32 Attempt = 0; Attempt < 12 && !bFound; ++Attempt)
		{
			bFound = Navigation->GetRandomReachablePointInRadius(Origin, 2500.f, Location) && FVector::Dist2D(Location.Location, Origin) > 900.f;
		}
		if (!bFound)
		{
			continue;
		}

		FActorSpawnParameters Parameters;
		Parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const FRotator Rotation(0.f, FMath::FRandRange(0.f, 360.f), 0.f);
		APawn* Pawn = World->SpawnActor<APawn>(Classes[Index % Classes.Num()], Location.Location + FVector(0.f, 0.f, 100.f), Rotation, Parameters);
		if (!Pawn)
		{
			continue;
		}
		if (!Pawn->GetController())
		{
			Pawn->SpawnDefaultController();
		}
		++Spawned;
	}

	StressSpawned += Spawned;
	LastDiscovery = -1000.0;
	AddTelemetryEvent(TEXT("TEST_OBCIAZENIA"), nullptr, FString(), FString(), -1.0, FString::Printf(TEXT("dodano %d"), Spawned));
	AddToast(FString::Printf(TEXT("Test obciążenia: +%d przeciwników"), Spawned), ThesisStyle::AccentColor());
}

void UThesisGameSubsystem::DiscoverEnemies()
{
	UWorld* World = GetWorld();
	APawn* Player = GetPlayerPawn();
	if (!World)
	{
		return;
	}

	bool bAdded = false;
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (!Pawn || Pawn == Player || Pawn->IsPlayerControlled() || Pawn->IsActorBeingDestroyed())
		{
			continue;
		}
		if (!Cast<AAIController>(Pawn->GetController()))
		{
			continue;
		}
		const bool bKnown = Enemies.ContainsByPredicate([Pawn](const FThesisEnemy& Enemy) { return Enemy.Pawn.Get() == Pawn; });
		double Health = 0.0;
		if (bKnown || !ThesisReflection::GetNumber(Pawn, HealthProperty, Health) || Health <= 0.0)
		{
			continue;
		}

		FThesisEnemy Enemy;
		Enemy.Pawn = Pawn;
		Enemy.PawnClass = Pawn->GetClass();
		Enemy.ClassName = Pawn->GetClass()->GetName();
		Enemy.ClassName.RemoveFromEnd(TEXT("_C"));
		Enemy.Kind = KindFromClass(Pawn->GetClass());
		int32& Counter = KindCounters.FindOrAdd(Enemy.Kind);
		++Counter;
		Enemy.DisplayName = FString::Printf(TEXT("%s #%d"), *ThesisText::KindLabel(Enemy.Kind), Counter);
		Enemy.bHostile = IsHostileToPlayer(Pawn);
		Enemy.Health = Health;
		Enemy.MaxHealth = FMath::Max(Health, 1.0);
		Enemy.BehaviorSince = GetNow();
		Enemy.bSpawnedByStressTest = Pawn->GetGameTimeSinceCreation() < 1.5f && SessionStart >= 0.0 && GetNow() - SessionStart > 1.5;
		ReadPerceptionConfig(Enemy, Pawn);
		LogEnemyTags(Enemy, Pawn);
		Enemies.Add(Enemy);
		bAdded = true;
	}

	if (bAdded && Enemies.Num() > 0 && GetHostileTotal() == 0)
	{
		for (FThesisEnemy& Enemy : Enemies)
		{
			Enemy.bHostile = true;
		}
	}
}

bool UThesisGameSubsystem::IsHostileToPlayer(APawn* EnemyPawn) const
{
	const APawn* Player = GetPlayerPawn();
	if (!Player || Player->Tags.Num() == 0)
	{
		return true;
	}

	TArray<FName> TargetTags;
	if (!ThesisReflection::GetNameArray(EnemyPawn, TargetTagsProperty, TargetTags))
	{
		ThesisReflection::GetNameArray(EnemyPawn->GetController(), TargetTagsProperty, TargetTags);
	}
	if (TargetTags.Num() == 0)
	{
		return true;
	}

	for (const FName& Tag : TargetTags)
	{
		if (Player->ActorHasTag(Tag))
		{
			return true;
		}
	}
	return false;
}

void UThesisGameSubsystem::LogEnemyTags(const FThesisEnemy& Enemy, APawn* EnemyPawn) const
{
	TArray<FName> TargetTags;
	if (!ThesisReflection::GetNameArray(EnemyPawn, TargetTagsProperty, TargetTags))
	{
		ThesisReflection::GetNameArray(EnemyPawn->GetController(), TargetTagsProperty, TargetTags);
	}
	const auto JoinNames = [](const TArray<FName>& Names)
	{
		TArray<FString> Strings;
		for (const FName& Name : Names)
		{
			Strings.Add(Name.ToString());
		}
		return FString::Join(Strings, TEXT(","));
	};
	const auto DumpTagProperties = [](const UObject* Object, const TCHAR* Owner)
	{
		if (!Object)
		{
			return;
		}
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			const FString Name = It->GetName();
			if (!Name.Contains(TEXT("Tag")) && !Name.Contains(TEXT("Team")))
			{
				continue;
			}
			FString Value;
			It->ExportTextItem_Direct(Value, It->ContainerPtrToValuePtr<void>(Object), nullptr, nullptr, PPF_None);
			UE_LOG(LogThesisUI, Log, TEXT("    %s.%s (%s) = %s"), Owner, *Name, *It->GetCPPType(), *Value.Left(200));
		}
	};
	DumpTagProperties(EnemyPawn, TEXT("pawn"));
	DumpTagProperties(EnemyPawn->GetController(), TEXT("controller"));
	for (const UActorComponent* Component : EnemyPawn->GetComponents())
	{
		DumpTagProperties(Component, *Component->GetName());
	}
	const APawn* Player = GetPlayerPawn();
	UE_LOG(LogThesisUI, Log, TEXT("Enemy %s class=%s tags=[%s] targetTags=[%s] playerTags=[%s] hostile=%d sight=%.0f/%.0f hearing=%.0f"),
		*Enemy.DisplayName, *Enemy.ClassName, *JoinNames(EnemyPawn->Tags), *JoinNames(TargetTags), Player ? *JoinNames(Player->Tags) : TEXT("-"), Enemy.bHostile ? 1 : 0, Enemy.SightRadius, Enemy.SightHalfAngle, Enemy.HearingRange);
}

void UThesisGameSubsystem::ReadPerceptionConfig(FThesisEnemy& Enemy, APawn* EnemyPawn) const
{
	AAIController* Controller = Cast<AAIController>(EnemyPawn->GetController());
	if (!Controller)
	{
		return;
	}

	UAIPerceptionComponent* Perception = Controller->GetAIPerceptionComponent();
	if (!Perception)
	{
		Perception = Controller->FindComponentByClass<UAIPerceptionComponent>();
	}
	if (!Perception)
	{
		return;
	}

	if (const UAISenseConfig_Sight* Sight = Cast<UAISenseConfig_Sight>(Perception->GetSenseConfig(UAISense::GetSenseID(UAISense_Sight::StaticClass()))))
	{
		Enemy.SightRadius = Sight->SightRadius;
		Enemy.SightHalfAngle = Sight->PeripheralVisionAngleDegrees;
	}
	if (const UAISenseConfig_Hearing* Hearing = Cast<UAISenseConfig_Hearing>(Perception->GetSenseConfig(UAISense::GetSenseID(UAISense_Hearing::StaticClass()))))
	{
		Enemy.HearingRange = Hearing->HearingRange;
	}
}

void UThesisGameSubsystem::UpdatePlayer()
{
	APawn* Player = GetPlayerPawn();
	if (!Player)
	{
		if (bPlayerSeen && bPlayerAlive)
		{
			bPlayerAlive = false;
			AddTelemetryEvent(TEXT("SMIERC_GRACZA"), nullptr, FString(), FString(), -1.0, TEXT("pionek usuniety"));
		}
		return;
	}

	bPlayerSeen = true;
	double Health = PlayerHealth;
	if (ThesisReflection::GetNumber(Player, HealthProperty, Health))
	{
		if (Health < PlayerHealth - 0.01)
		{
			DamageFlashUntil = GetRealTime() + 0.45;
		}
		PlayerHealth = Health;
		PlayerMaxHealth = FMath::Max(PlayerMaxHealth, Health);
		if (bGodMode && Health > 0.0 && Health < PlayerMaxHealth)
		{
			ThesisReflection::SetNumber(Player, HealthProperty, PlayerMaxHealth);
		}
	}

	bool bAlive = PlayerHealth > 0.0;
	ThesisReflection::CallBoolFunction(Player, IsAliveFunction, bAlive);
	if (bPlayerAlive && !bAlive)
	{
		AddTelemetryEvent(TEXT("SMIERC_GRACZA"), nullptr, FString(), FString(), -1.0, FString());
	}
	bPlayerAlive = bAlive;
}

void UThesisGameSubsystem::UpdateEnemies()
{
	for (FThesisEnemy& Enemy : Enemies)
	{
		if (Enemy.bAlive)
		{
			UpdateEnemy(Enemy);
		}
	}
}

FString UThesisGameSubsystem::ReadBehavior(UBlackboardComponent* Blackboard) const
{
	const UBlackboardData* Asset = Blackboard ? Blackboard->GetBlackboardAsset() : nullptr;
	if (!Asset)
	{
		return FString();
	}

	const FBlackboard::FKey KeyId = Blackboard->GetKeyID(BehaviorKey);
	if (KeyId == FBlackboard::InvalidKey)
	{
		return FString();
	}

	const uint8 Value = Blackboard->GetValueAsEnum(BehaviorKey);
	const FBlackboardEntry* Entry = Asset->GetKey(KeyId);
	const UBlackboardKeyType_Enum* KeyType = Entry ? Cast<UBlackboardKeyType_Enum>(Entry->KeyType) : nullptr;
	if (KeyType && KeyType->EnumType)
	{
		return KeyType->EnumType->GetDisplayNameTextByValue(Value).ToString();
	}
	return FString::FromInt(Value);
}

void UThesisGameSubsystem::UpdateEnemy(FThesisEnemy& Enemy)
{
	APawn* Pawn = Enemy.Pawn.Get();
	if (!Pawn || Pawn->IsActorBeingDestroyed())
	{
		MarkEnemyDead(Enemy, TEXT("aktor usuniety"));
		return;
	}

	double Health = Enemy.Health;
	if (ThesisReflection::GetNumber(Pawn, HealthProperty, Health))
	{
		if (Health < Enemy.Health - 0.01)
		{
			Enemy.HitFlashUntil = GetRealTime() + 0.25;
			if (Enemy.bHostile && bPlayerAlive)
			{
				HitMarkerUntil = GetRealTime() + 0.15;
			}
		}
		Enemy.Health = Health;
		Enemy.MaxHealth = FMath::Max(Enemy.MaxHealth, Health);
	}

	bool bAlive = Enemy.Health > 0.0;
	ThesisReflection::CallBoolFunction(Pawn, IsAliveFunction, bAlive);
	if (!bAlive)
	{
		MarkEnemyDead(Enemy, FString());
		return;
	}

	APawn* Player = GetPlayerPawn();
	if (Player)
	{
		Enemy.LastDistanceToPlayer = FVector::Dist(Pawn->GetActorLocation(), Player->GetActorLocation());
	}

	AAIController* Controller = Cast<AAIController>(Pawn->GetController());
	UBlackboardComponent* Blackboard = Controller ? Controller->GetBlackboardComponent() : nullptr;
	if (Blackboard)
	{
		const FString Behavior = ReadBehavior(Blackboard);
		if (!Behavior.IsEmpty() && Behavior != Enemy.BehaviorId)
		{
			OnBehaviorChanged(Enemy, Behavior);
		}

		const bool bTargetsPlayer = Player && Blackboard->GetValueAsObject(TargetKey) == Player;
		if (bTargetsPlayer != Enemy.bTargetsPlayer)
		{
			OnTargetChanged(Enemy, bTargetsPlayer);
		}
	}

	UpdatePlayerSightTimer(Enemy, Pawn);
}

void UThesisGameSubsystem::UpdatePlayerSightTimer(FThesisEnemy& Enemy, APawn* EnemyPawn)
{
	if (Enemy.bTargetsPlayer || !bPlayerAlive || Enemy.SightRadius <= 0.f)
	{
		Enemy.PlayerInSightSince = -1.0;
		return;
	}

	if (IsPlayerInSightCone(Enemy, EnemyPawn))
	{
		if (Enemy.PlayerInSightSince < 0.0)
		{
			Enemy.PlayerInSightSince = GetNow();
		}
	}
	else
	{
		Enemy.PlayerInSightSince = -1.0;
	}
}

bool UThesisGameSubsystem::IsPlayerInSightCone(const FThesisEnemy& Enemy, APawn* EnemyPawn) const
{
	APawn* Player = GetPlayerPawn();
	UWorld* World = GetWorld();
	if (!Player || !World)
	{
		return false;
	}

	FVector EyeLocation;
	FRotator EyeRotation;
	if (AController* Controller = EnemyPawn->GetController())
	{
		Controller->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	}
	else
	{
		EnemyPawn->GetActorEyesViewPoint(EyeLocation, EyeRotation);
	}

	const FVector PlayerLocation = Player->GetActorLocation();
	const FVector ToPlayer = PlayerLocation - EyeLocation;
	const double Distance = ToPlayer.Size();
	if (Distance > Enemy.SightRadius || Distance < KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const double CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(Enemy.SightHalfAngle, 0.f, 180.f)));
	if (FVector::DotProduct(EyeRotation.Vector(), ToPlayer / Distance) < CosHalfAngle)
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ThesisSightCheck), true, EnemyPawn);
	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, EyeLocation, PlayerLocation, ECC_Visibility, Params);
	return !bHit || Hit.GetActor() == Player;
}

void UThesisGameSubsystem::OnBehaviorChanged(FThesisEnemy& Enemy, const FString& NewBehavior)
{
	const double Now = GetNow();
	const FString From = Enemy.BehaviorId;

	if (From.IsEmpty())
	{
		AddTelemetryEvent(TEXT("STAN_POCZATKOWY"), &Enemy, FString(), NewBehavior, -1.0, FString());
	}
	else
	{
		Enemy.TimeInBehavior.FindOrAdd(From) += Now - Enemy.BehaviorSince;
		++Enemy.Transitions;

		const bool bOscillation = Enemy.LastTransitionFrom == NewBehavior && Enemy.LastTransitionTo == From && Now - Enemy.LastTransitionAt < OscillationWindow;
		if (bOscillation)
		{
			++Enemy.Oscillations;
		}

		Enemy.LastTransitionFrom = From;
		Enemy.LastTransitionTo = NewBehavior;
		Enemy.LastTransitionAt = Now;

		AddTelemetryEvent(TEXT("ZMIANA_ZACHOWANIA"), &Enemy, From, NewBehavior, -1.0, bOscillation ? TEXT("oscylacja") : FString());
		AddLog(FString::Printf(TEXT("%s: %s → %s (%s)"), *Enemy.DisplayName, *ThesisText::BehaviorLabel(From), *ThesisText::BehaviorLabel(NewBehavior), *ThesisText::Meters(Enemy.LastDistanceToPlayer)), ThesisText::BehaviorGroup(NewBehavior));
	}

	Enemy.BehaviorId = NewBehavior;
	Enemy.BehaviorSince = Now;
}

void UThesisGameSubsystem::OnTargetChanged(FThesisEnemy& Enemy, bool bNowTargetsPlayer)
{
	Enemy.bTargetsPlayer = bNowTargetsPlayer;
	if (bNowTargetsPlayer)
	{
		if (Enemy.PlayerInSightSince >= 0.0)
		{
			const double Reaction = GetNow() - Enemy.PlayerInSightSince;
			Enemy.ReactionTimes.Add(Reaction);
			AddTelemetryEvent(TEXT("WYKRYCIE_GRACZA"), &Enemy, Enemy.BehaviorId, FString(), Reaction, TEXT("wzrok"));
			AddLog(FString::Printf(TEXT("%s wykrył gracza po %s"), *Enemy.DisplayName, *ThesisText::Seconds(Reaction, 2)), EThesisBehaviorGroup::Attack);
		}
		else
		{
			++Enemy.DetectionsWithoutSight;
			AddTelemetryEvent(TEXT("WYKRYCIE_GRACZA"), &Enemy, Enemy.BehaviorId, FString(), -1.0, TEXT("bez kontaktu wzrokowego (sluch lub obrazenia)"));
			AddLog(FString::Printf(TEXT("%s wykrył gracza (słuch/obrażenia)"), *Enemy.DisplayName), EThesisBehaviorGroup::Attack);
		}
		AddToast(FString::Printf(TEXT("Zauważył cię: %s"), *Enemy.DisplayName), ThesisStyle::DangerColor());
		Enemy.PlayerInSightSince = -1.0;
	}
	else
	{
		AddTelemetryEvent(TEXT("UTRATA_CELU"), &Enemy, Enemy.BehaviorId, FString(), -1.0, FString());
		AddLog(FString::Printf(TEXT("%s stracił cel"), *Enemy.DisplayName), EThesisBehaviorGroup::Neutral);
	}
}

void UThesisGameSubsystem::MarkEnemyDead(FThesisEnemy& Enemy, const FString& Reason)
{
	if (!Enemy.bAlive)
	{
		return;
	}

	const double Now = GetNow();
	if (!Enemy.BehaviorId.IsEmpty())
	{
		Enemy.TimeInBehavior.FindOrAdd(Enemy.BehaviorId) += Now - Enemy.BehaviorSince;
	}
	Enemy.BehaviorSince = Now;
	Enemy.bAlive = false;
	Enemy.bTargetsPlayer = false;
	Enemy.DiedAt = Now;
	Enemy.Health = 0.0;

	if (Enemy.bHostile)
	{
		++Kills;
	}

	AddTelemetryEvent(TEXT("ELIMINACJA"), &Enemy, Enemy.BehaviorId, TEXT("Dead"), -1.0, Reason);
	AddLog(FString::Printf(TEXT("%s wyeliminowany"), *Enemy.DisplayName), EThesisBehaviorGroup::Dead);
	AddToast(FString::Printf(TEXT("Wyeliminowano: %s"), *Enemy.DisplayName), ThesisStyle::GoodColor());
}

void UThesisGameSubsystem::CheckEndConditions()
{
	const double Real = GetRealTime();
	if (BenchmarkSeconds > 0.0)
	{
		return;
	}

	if (!bPlayerAlive && bPlayerSeen)
	{
		if (EndScreenAt < 0.0)
		{
			EndScreenAt = Real + 2.0;
			bPendingWin = false;
		}
	}
	else if (GetHostileTotal() > 0 && GetHostileAlive() == 0)
	{
		if (EndScreenAt < 0.0)
		{
			EndScreenAt = Real + 1.5;
			bPendingWin = true;
		}
	}

	if (EndScreenAt >= 0.0 && Real >= EndScreenAt)
	{
		FinishGame(bPendingWin);
	}
}

void UThesisGameSubsystem::FinishGame(bool bWon)
{
	EndScreenAt = -1.0;
	SessionEnd = GetNow();
	AddTelemetryEvent(TEXT("KONIEC"), nullptr, FString(), FString(), -1.0, bWon ? TEXT("zwyciestwo") : TEXT("porazka"));
	SetGameState(bWon ? EThesisGameState::Won : EThesisGameState::Lost);
	SaveTelemetry(false);
}

void UThesisGameSubsystem::CaptureScreenshot(const FString& Name) const
{
	const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("ThesisTour") / (Name + TEXT(".png")));
	FScreenshotRequest::RequestScreenshot(Path, true, false);
	UE_LOG(LogThesisUI, Log, TEXT("Screenshot requested: %s"), *Path);
}

void UThesisGameSubsystem::MoveToTourSpot()
{
	FString Spot;
	if (!FParse::Value(FCommandLine::Get(), TEXT("ThesisSpot="), Spot, false))
	{
		return;
	}

	TArray<FString> Parts;
	Spot.ParseIntoArray(Parts, TEXT(","));
	APawn* Player = GetPlayerPawn();
	APlayerController* Controller = GetPlayerController();
	if (Parts.Num() < 4 || !Player || !Controller)
	{
		return;
	}

	const FVector Location(FCString::Atod(*Parts[0]), FCString::Atod(*Parts[1]), FCString::Atod(*Parts[2]));
	const double Yaw = FCString::Atod(*Parts[3]);
	Player->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	Player->SetActorRotation(FRotator(0.0, Yaw, 0.0));
	Controller->SetControlRotation(FRotator(-12.0, Yaw, 0.0));
	UE_LOG(LogThesisUI, Log, TEXT("Tour: player moved to %s yaw %.0f"), *Location.ToString(), Yaw);
}

void UThesisGameSubsystem::UpdateAutomation()
{
	if (!IsAutomationActive() || AutomationStart < 0.0)
	{
		return;
	}

	if (bTourMode)
	{
		UpdateTour(GetRealTime() - AutomationStart);
	}
	else
	{
		UpdateBenchmark();
	}
}

void UThesisGameSubsystem::UpdateTour(double Elapsed)
{
	struct FTourStep
	{
		double Delay;
		TFunction<void(UThesisGameSubsystem&)> Action;
	};

	static const TArray<FTourStep> Steps = {
		{ 6.0, [](UThesisGameSubsystem& S) { S.CaptureScreenshot(TEXT("01_menu_glowne")); } },
		{ 1.5, [](UThesisGameSubsystem& S) { S.ShowControls(); } },
		{ 1.5, [](UThesisGameSubsystem& S) { S.CaptureScreenshot(TEXT("02_sterowanie")); } },
		{ 1.5, [](UThesisGameSubsystem& S) { S.CloseControls(); S.StartGame(); S.MoveToTourSpot(); } },
		{ 12.0, [](UThesisGameSubsystem& S) { S.CaptureScreenshot(TEXT("03_hud")); } },
		{ 1.0, [](UThesisGameSubsystem& S) { S.ToggleDebugPanel(); } },
		{ 10.0, [](UThesisGameSubsystem& S) { S.CaptureScreenshot(TEXT("04_panel_ai")); } },
		{ 1.0, [](UThesisGameSubsystem& S) { S.SpawnStressWave(); } },
		{ 12.0, [](UThesisGameSubsystem& S) { S.CaptureScreenshot(TEXT("05_test_obciazenia")); } },
		{ 1.0, [](UThesisGameSubsystem& S) { S.TogglePause(); } },
		{ 2.0, [](UThesisGameSubsystem& S) { S.CaptureScreenshot(TEXT("06_pauza")); } },
		{ 1.5, [](UThesisGameSubsystem& S) { S.Resume(); } },
		{ 1.0, [](UThesisGameSubsystem& S) { S.AutomationOutcome = TEXT("pokaz_interfejsu"); S.FinishGame(true); } },
		{ 2.0, [](UThesisGameSubsystem& S) { S.CaptureScreenshot(TEXT("07_ekran_konca")); } },
		{ 2.5, [](UThesisGameSubsystem& S) { S.QuitGame(); } }
	};

	if (TourStep >= Steps.Num())
	{
		return;
	}
	if (LastTourStepFrame == 0)
	{
		LastTourStepFrame = GFrameCounter;
		LastTourStepTime = GetRealTime();
	}
	const bool bEnoughTime = GetRealTime() - LastTourStepTime >= Steps[TourStep].Delay;
	const bool bEnoughFrames = GFrameCounter - LastTourStepFrame >= 20;
	if (bEnoughTime && bEnoughFrames)
	{
		UE_LOG(LogThesisUI, Log, TEXT("Tour step %d at %.1f s"), TourStep, Elapsed);
		Steps[TourStep++].Action(*this);
		LastTourStepTime = GetRealTime();
		LastTourStepFrame = GFrameCounter;
	}
}

void UThesisGameSubsystem::UpdateBenchmark()
{
	if (GameState != EThesisGameState::Playing)
	{
		return;
	}

	if (GetSessionTime() >= BenchmarkSeconds)
	{
		AutomationOutcome = TEXT("pomiar_automatyczny");
		SessionEnd = GetNow();
		AddTelemetryEvent(TEXT("KONIEC"), nullptr, FString(), FString(), -1.0, AutomationOutcome);
		CaptureScreenshot(TEXT("benchmark_koniec"));
		SaveTelemetry(false);
		BenchmarkSeconds = 0.0;
		QuitGame();
		return;
	}

	int32 AliveCount = 0;
	for (const FThesisEnemy& Enemy : Enemies)
	{
		AliveCount += Enemy.bAlive ? 1 : 0;
	}

	const double WaveInterval = FMath::Max(5.0, static_cast<double>(CVarThesisBenchmarkWave.GetValueOnGameThread()));
	if (GetNow() - LastBenchmarkWave >= WaveInterval && AliveCount < CVarThesisBenchmarkMax.GetValueOnGameThread())
	{
		LastBenchmarkWave = GetNow();
		SpawnStressWave();
	}
}

void UThesisGameSubsystem::AddLog(const FString& Text, EThesisBehaviorGroup Group)
{
	FThesisLogLine Line;
	Line.Time = GetSessionTime();
	Line.Text = Text;
	Line.Group = Group;
	DecisionLog.Insert(Line, 0);
	if (DecisionLog.Num() > MaxLogLines)
	{
		DecisionLog.SetNum(MaxLogLines);
	}
}

void UThesisGameSubsystem::AddToast(const FString& Text, const FLinearColor& Color)
{
	FThesisToast Toast;
	Toast.Text = Text;
	Toast.Color = Color;
	Toast.ShownAt = GetRealTime();
	Toasts.Add(Toast);
	if (Toasts.Num() > MaxToasts)
	{
		Toasts.RemoveAt(0);
	}
}

void UThesisGameSubsystem::AddTelemetryEvent(const FString& EventName, const FThesisEnemy* Enemy, const FString& From, const FString& To, double ReactionTime, const FString& Detail)
{
	int32 AliveCount = 0;
	for (const FThesisEnemy& Other : Enemies)
	{
		AliveCount += Other.bAlive ? 1 : 0;
	}
	Telemetry.AddEvent(GetSessionTime(), EventName, Enemy, From, To, PlayerHealth, AliveCount, CurrentFps, ReactionTime, Detail);
}
