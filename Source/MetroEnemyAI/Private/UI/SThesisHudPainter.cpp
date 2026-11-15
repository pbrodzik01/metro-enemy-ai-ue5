#include "UI/SThesisHudPainter.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "ThesisGameSubsystem.h"
#include "ThesisText.h"
#include "UI/ThesisStyle.h"

namespace
{
	constexpr double Margin = 28.0;
	constexpr double StatsWidth = 260.0;
	constexpr double StatsRowHeight = 26.0;
	constexpr int32 StatsRows = 3;
	constexpr double DebugWidth = 640.0;
	constexpr double ToastLifetime = 3.5;
	constexpr int32 MaxDebugRows = 14;

	FLinearColor WithAlpha(FLinearColor Color, float Alpha)
	{
		Color.A *= Alpha;
		return Color;
	}

	double StatsHeight()
	{
		return 16.0 + StatsRows * StatsRowHeight;
	}
}

void SThesisHudPainter::Construct(const FArguments& InArgs)
{
	Subsystem = InArgs._Subsystem;
	ForceVolatile(true);
}

FVector2D SThesisHudPainter::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	return FVector2D(1.0, 1.0);
}

void SThesisHudPainter::Box(FPaintContext& Context, const FVector2D& Position, const FVector2D& Size, const FLinearColor& Color, const FSlateBrush* Brush) const
{
	if (Size.X <= 0.0 || Size.Y <= 0.0)
	{
		return;
	}
	FSlateDrawElement::MakeBox(Context.Elements, ++Context.Layer, Context.Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)), Brush, ESlateDrawEffect::None, Color);
}

FVector2D SThesisHudPainter::Measure(const FString& Value, const FSlateFontInfo& Font) const
{
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	return FVector2D(FontMeasure->Measure(Value, Font));
}

FVector2D SThesisHudPainter::Text(FPaintContext& Context, const FString& Value, const FVector2D& Position, const FSlateFontInfo& Font, const FLinearColor& Color, bool bShadow) const
{
	const FVector2D Size = Measure(Value, Font);
	if (bShadow)
	{
		FSlateDrawElement::MakeText(Context.Elements, ++Context.Layer, Context.Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position + FVector2D(1.0, 1.0))), Value, Font, ESlateDrawEffect::None, WithAlpha(ThesisStyle::ShadowColor(), Color.A));
	}
	FSlateDrawElement::MakeText(Context.Elements, ++Context.Layer, Context.Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(Position)), Value, Font, ESlateDrawEffect::None, Color);
	return Size;
}

void SThesisHudPainter::Bar(FPaintContext& Context, const FVector2D& Position, const FVector2D& Size, double Fraction, const FLinearColor& FillColor) const
{
	Box(Context, Position - FVector2D(1.0, 1.0), Size + FVector2D(2.0, 2.0), FLinearColor(0.f, 0.f, 0.f, 0.6f), ThesisStyle::RoundedSmall());
	Box(Context, Position, FVector2D(Size.X * FMath::Clamp(Fraction, 0.0, 1.0), Size.Y), FillColor, ThesisStyle::RoundedSmall());
}

int32 SThesisHudPainter::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const UThesisGameSubsystem* Owner = Subsystem.Get();
	if (!Owner || !Owner->HasSessionStarted() || Owner->GetGameState() == EThesisGameState::MainMenu)
	{
		return LayerId;
	}

	FPaintContext Context{ AllottedGeometry, OutDrawElements, LayerId };
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();

	PaintEnemyLabels(Context, *Owner, LocalSize);
	PaintDamageVignette(Context, *Owner, LocalSize);
	PaintHitMarker(Context, *Owner, LocalSize);
	PaintObjective(Context, *Owner);
	PaintStats(Context, *Owner, LocalSize);
	PaintPlayerPanel(Context, *Owner, LocalSize);
	PaintHints(Context, LocalSize);
	PaintToasts(Context, *Owner, LocalSize);
	if (Owner->IsDebugPanelVisible())
	{
		PaintDebugPanel(Context, *Owner, LocalSize);
	}

	return Context.Layer;
}

void SThesisHudPainter::PaintEnemyLabels(FPaintContext& Context, const UThesisGameSubsystem& Owner, const FVector2D& LocalSize) const
{
	if (Owner.GetLabelMode() == EThesisLabelMode::Off)
	{
		return;
	}

	UWorld* World = Owner.GetWorld();
	APlayerController* Controller = Owner.GetPlayerController();
	UGameViewportClient* Viewport = World ? World->GetGameViewport() : nullptr;
	if (!Controller || !Viewport)
	{
		return;
	}

	FVector2D ViewportSize;
	Viewport->GetViewportSize(ViewportSize);
	if (ViewportSize.X < 1.0 || ViewportSize.Y < 1.0)
	{
		return;
	}

	const FVector2D ToLocal(LocalSize.X / ViewportSize.X, LocalSize.Y / ViewportSize.Y);
	const FVector CameraLocation = Controller->PlayerCameraManager ? Controller->PlayerCameraManager->GetCameraLocation() : Controller->GetFocalLocation();

	struct FLabelItem
	{
		const FThesisEnemy* Enemy;
		FVector2D Anchor;
		double Distance;
	};

	TArray<FLabelItem> Items;
	for (const FThesisEnemy& Enemy : Owner.GetEnemies())
	{
		APawn* Pawn = Enemy.Pawn.Get();
		if (!Enemy.bAlive || !Pawn)
		{
			continue;
		}

		double HalfHeight = 90.0;
		if (const ACharacter* Character = Cast<ACharacter>(Pawn))
		{
			if (const UCapsuleComponent* Capsule = Character->GetCapsuleComponent())
			{
				HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
			}
		}

		const FVector Head = Pawn->GetActorLocation() + FVector(0.0, 0.0, HalfHeight + 28.0);
		const double Distance = FVector::Dist(CameraLocation, Head);
		if (Distance > 8000.0)
		{
			continue;
		}

		if (Owner.GetLabelMode() == EThesisLabelMode::VisibleOnly)
		{
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ThesisLabelVisibility), false);
			Params.AddIgnoredActor(Pawn);
			Params.AddIgnoredActor(Controller->GetPawn());
			if (World->LineTraceTestByChannel(CameraLocation, Head, ECC_Visibility, Params))
			{
				continue;
			}
		}

		FVector2D Screen;
		if (!Controller->ProjectWorldLocationToScreen(Head, Screen, false))
		{
			continue;
		}

		const FVector2D Anchor = Screen * ToLocal;
		if (Anchor.X < -120.0 || Anchor.Y < -120.0 || Anchor.X > LocalSize.X + 120.0 || Anchor.Y > LocalSize.Y + 120.0)
		{
			continue;
		}
		Items.Add({ &Enemy, Anchor, Distance });
	}

	Items.Sort([](const FLabelItem& A, const FLabelItem& B) { return A.Distance > B.Distance; });
	for (const FLabelItem& Item : Items)
	{
		const float Scale = static_cast<float>(FMath::GetMappedRangeValueClamped(FVector2D(500.0, 5000.0), FVector2D(1.0, 0.72), Item.Distance));
		PaintEnemyLabel(Context, Owner, *Item.Enemy, Item.Anchor, Scale);
	}
}

void SThesisHudPainter::PaintEnemyLabel(FPaintContext& Context, const UThesisGameSubsystem& Owner, const FThesisEnemy& Enemy, const FVector2D& Anchor, float Scale) const
{
	const EThesisBehaviorGroup Group = ThesisText::BehaviorGroup(Enemy.BehaviorId);
	FString Chip = ThesisText::BehaviorLabel(Enemy.BehaviorId);
	if (Owner.IsDebugPanelVisible())
	{
		Chip += TEXT("  ") + ThesisText::Seconds(Owner.GetNow() - Enemy.BehaviorSince, 1);
	}

	const FSlateFontInfo ChipFont = ThesisStyle::Font(FMath::Max(8, FMath::RoundToInt(11.f * Scale)), true);
	const FVector2D ChipTextSize = Measure(Chip, ChipFont);
	const FVector2D Padding(9.0 * Scale, 3.0 * Scale);
	const FVector2D ChipSize = ChipTextSize + Padding * 2.0;
	const double BarWidth = FMath::Max(ChipSize.X, 84.0 * Scale);
	const double BarHeight = FMath::Max(3.0, 5.0 * Scale);

	const FVector2D BarPosition(Anchor.X - BarWidth * 0.5, Anchor.Y - BarHeight);
	const FVector2D ChipPosition(Anchor.X - ChipSize.X * 0.5, BarPosition.Y - 4.0 * Scale - ChipSize.Y);

	Box(Context, ChipPosition, ChipSize, WithAlpha(ThesisStyle::BehaviorColor(Group), 0.92f), ThesisStyle::Pill());
	Text(Context, Chip, ChipPosition + Padding, ChipFont, ThesisStyle::TextColor(), false);

	if (Enemy.bTargetsPlayer)
	{
		const FVector2D MarkerSize(ChipSize.Y, ChipSize.Y);
		const FVector2D MarkerPosition(ChipPosition.X - MarkerSize.X - 4.0 * Scale, ChipPosition.Y);
		Box(Context, MarkerPosition, MarkerSize, ThesisStyle::DangerColor(), ThesisStyle::Pill());
		const FVector2D MarkSize = Measure(TEXT("!"), ChipFont);
		Text(Context, TEXT("!"), MarkerPosition + (MarkerSize - MarkSize) * 0.5, ChipFont, ThesisStyle::TextColor(), false);
	}

	const FString Name = Enemy.bHostile ? Enemy.DisplayName : Enemy.DisplayName + TEXT(" · inna drużyna");
	const FSlateFontInfo NameFont = ThesisStyle::Font(FMath::Max(8, FMath::RoundToInt(10.f * Scale)), false);
	const FVector2D NameSize = Measure(Name, NameFont);
	Text(Context, Name, FVector2D(Anchor.X - NameSize.X * 0.5, ChipPosition.Y - NameSize.Y - 2.0 * Scale), NameFont, WithAlpha(ThesisStyle::TextColor(), 0.92f));

	const double Fraction = Enemy.MaxHealth > 0.0 ? Enemy.Health / Enemy.MaxHealth : 0.0;
	if (Owner.GetRealTime() < Enemy.HitFlashUntil)
	{
		Box(Context, BarPosition - FVector2D(3.0, 3.0), FVector2D(BarWidth + 6.0, BarHeight + 6.0), FLinearColor(1.f, 1.f, 1.f, 0.85f), ThesisStyle::RoundedSmall());
	}
	Bar(Context, BarPosition, FVector2D(BarWidth, BarHeight), Fraction, ThesisStyle::HealthColor(Fraction));
}

void SThesisHudPainter::PaintPlayerPanel(FPaintContext& Context, const UThesisGameSubsystem& Owner, const FVector2D& LocalSize) const
{
	const FVector2D Size(340.0, 66.0);
	const FVector2D Position(Margin, LocalSize.Y - Margin - Size.Y);
	Box(Context, Position, Size, ThesisStyle::PanelColor(), ThesisStyle::Rounded());

	Text(Context, TEXT("ZDROWIE"), Position + FVector2D(18.0, 12.0), ThesisStyle::Font(10, true), ThesisStyle::TextDimColor());

	const double Health = FMath::Max(0.0, Owner.GetPlayerHealth());
	const double MaxHealth = FMath::Max(1.0, Owner.GetPlayerMaxHealth());
	const FString Value = Owner.IsPlayerAlive()
		? FString::Printf(TEXT("%d / %d"), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth))
		: FString(TEXT("WYELIMINOWANY"));
	const FSlateFontInfo ValueFont = ThesisStyle::Font(13, true);
	const FVector2D ValueSize = Measure(Value, ValueFont);
	Text(Context, Value, FVector2D(Position.X + Size.X - 18.0 - ValueSize.X, Position.Y + 9.0), ValueFont, Owner.IsPlayerAlive() ? ThesisStyle::TextColor() : ThesisStyle::DangerColor());

	const double Fraction = Health / MaxHealth;
	Bar(Context, Position + FVector2D(18.0, 40.0), FVector2D(Size.X - 36.0, 12.0), Fraction, ThesisStyle::HealthColor(Fraction));
}

void SThesisHudPainter::PaintObjective(FPaintContext& Context, const UThesisGameSubsystem& Owner) const
{
	const FString Title = TEXT("CEL");
	const int32 Remaining = Owner.GetHostileAlive();
	const FString Goal = Remaining > 0
		? FString::Printf(TEXT("Wyeliminuj wrogów na stacji · pozostało: %d"), Remaining)
		: FString(TEXT("Stacja oczyszczona"));

	const FSlateFontInfo TitleFont = ThesisStyle::Font(10, true);
	const FSlateFontInfo GoalFont = ThesisStyle::Font(13, true);
	const FVector2D TitleSize = Measure(Title, TitleFont);
	const FVector2D GoalSize = Measure(Goal, GoalFont);
	const FVector2D Size(18.0 + TitleSize.X + 12.0 + GoalSize.X + 18.0, 40.0);
	const FVector2D Position(Margin, Margin);

	Box(Context, Position, Size, ThesisStyle::PanelColor(), ThesisStyle::Rounded());
	Box(Context, Position + FVector2D(10.0, 10.0), FVector2D(TitleSize.X + 12.0, 20.0), ThesisStyle::AccentColor(), ThesisStyle::RoundedSmall());
	Text(Context, Title, Position + FVector2D(16.0, 20.0 - TitleSize.Y * 0.5), TitleFont, FLinearColor(0.02f, 0.02f, 0.02f, 1.f), false);
	Text(Context, Goal, Position + FVector2D(18.0 + TitleSize.X + 12.0, 20.0 - GoalSize.Y * 0.5), GoalFont, ThesisStyle::TextColor());
}

void SThesisHudPainter::PaintStats(FPaintContext& Context, const UThesisGameSubsystem& Owner, const FVector2D& LocalSize) const
{
	const FVector2D Position(LocalSize.X - Margin - StatsWidth, Margin);
	Box(Context, Position, FVector2D(StatsWidth, StatsHeight()), ThesisStyle::PanelColor(), ThesisStyle::Rounded());

	const FString Labels[StatsRows] = { TEXT("PRZECIWNICY"), TEXT("ELIMINACJE"), TEXT("CZAS") };
	const FString Values[StatsRows] = {
		FString::Printf(TEXT("%d / %d"), Owner.GetHostileAlive(), Owner.GetHostileTotal()),
		FString::FromInt(Owner.GetKills()),
		ThesisText::Clock(Owner.GetSessionTime())
	};

	const FSlateFontInfo LabelFont = ThesisStyle::Font(10, true);
	const FSlateFontInfo ValueFont = ThesisStyle::Font(15, true);
	for (int32 Index = 0; Index < StatsRows; ++Index)
	{
		const double RowCenter = Position.Y + 8.0 + StatsRowHeight * (Index + 0.5);
		const FVector2D LabelSize = Measure(Labels[Index], LabelFont);
		const FVector2D ValueSize = Measure(Values[Index], ValueFont);
		Text(Context, Labels[Index], FVector2D(Position.X + 18.0, RowCenter - LabelSize.Y * 0.5), LabelFont, ThesisStyle::TextDimColor());
		Text(Context, Values[Index], FVector2D(Position.X + StatsWidth - 18.0 - ValueSize.X, RowCenter - ValueSize.Y * 0.5), ValueFont, ThesisStyle::TextColor());
	}
}

void SThesisHudPainter::PaintHints(FPaintContext& Context, const FVector2D& LocalSize) const
{
	struct FHint
	{
		const TCHAR* Key;
		const TCHAR* Value;
	};
	const FHint Hints[] = {
		{ TEXT("F1"), TEXT("panel AI") },
		{ TEXT("F2"), TEXT("etykiety") },
		{ TEXT("F3"), TEXT("zapis CSV") },
		{ TEXT("F5"), TEXT("test obciążenia") },
		{ TEXT("P"), TEXT("pauza") }
	};

	const FSlateFontInfo KeyFont = ThesisStyle::Font(10, true);
	const FSlateFontInfo LabelFont = ThesisStyle::Font(10, false);

	double TotalWidth = 0.0;
	for (const FHint& Hint : Hints)
	{
		TotalWidth += Measure(Hint.Key, KeyFont).X + 12.0 + 6.0 + Measure(Hint.Value, LabelFont).X + 16.0;
	}

	double X = LocalSize.X - Margin - TotalWidth;
	const double Y = LocalSize.Y - Margin - 22.0;
	for (const FHint& Hint : Hints)
	{
		const FVector2D KeySize = Measure(Hint.Key, KeyFont);
		Box(Context, FVector2D(X, Y), FVector2D(KeySize.X + 12.0, 22.0), WithAlpha(ThesisStyle::PanelStrongColor(), 0.9f), ThesisStyle::RoundedSmall());
		Text(Context, Hint.Key, FVector2D(X + 6.0, Y + 11.0 - KeySize.Y * 0.5), KeyFont, ThesisStyle::AccentColor(), false);
		X += KeySize.X + 12.0 + 6.0;

		const FVector2D LabelSize = Text(Context, Hint.Value, FVector2D(X, Y + 11.0 - Measure(Hint.Value, LabelFont).Y * 0.5), LabelFont, ThesisStyle::TextColor());
		X += LabelSize.X + 16.0;
	}
}

void SThesisHudPainter::PaintToasts(FPaintContext& Context, const UThesisGameSubsystem& Owner, const FVector2D& LocalSize) const
{
	const double Real = Owner.GetRealTime();
	const FSlateFontInfo Font = ThesisStyle::Font(13, true);
	double Y = 96.0;

	for (int32 Index = Owner.GetToasts().Num() - 1; Index >= 0; --Index)
	{
		const FThesisToast& Toast = Owner.GetToasts()[Index];
		const double Age = Real - Toast.ShownAt;
		float Alpha = 1.f;
		if (Age < 0.2)
		{
			Alpha = static_cast<float>(Age / 0.2);
		}
		else if (Age > ToastLifetime - 0.6)
		{
			Alpha = static_cast<float>(FMath::Max(0.0, (ToastLifetime - Age) / 0.6));
		}

		const FVector2D TextSize = Measure(Toast.Text, Font);
		const FVector2D Size(TextSize.X + 46.0, TextSize.Y + 16.0);
		const FVector2D Position((LocalSize.X - Size.X) * 0.5, Y);
		Box(Context, Position, Size, WithAlpha(ThesisStyle::PanelStrongColor(), Alpha), ThesisStyle::Pill());
		Box(Context, Position + FVector2D(14.0, Size.Y * 0.5 - 5.0), FVector2D(10.0, 10.0), WithAlpha(Toast.Color, Alpha), ThesisStyle::Pill());
		Text(Context, Toast.Text, Position + FVector2D(32.0, 8.0), Font, WithAlpha(ThesisStyle::TextColor(), Alpha));
		Y += Size.Y + 8.0;
	}
}

void SThesisHudPainter::PaintHitMarker(FPaintContext& Context, const UThesisGameSubsystem& Owner, const FVector2D& LocalSize) const
{
	const double Remaining = Owner.GetHitMarkerUntil() - Owner.GetRealTime();
	if (Remaining <= 0.0)
	{
		return;
	}

	const float Alpha = static_cast<float>(FMath::Clamp(Remaining / 0.15, 0.0, 1.0));
	const FVector2D Center = LocalSize * 0.5;
	for (const FVector2D Direction : { FVector2D(-1.0, -1.0), FVector2D(1.0, -1.0), FVector2D(-1.0, 1.0), FVector2D(1.0, 1.0) })
	{
		Box(Context, Center + Direction * 10.0 - FVector2D(2.5, 2.5), FVector2D(5.0, 5.0), FLinearColor(1.f, 1.f, 1.f, Alpha), ThesisStyle::Solid());
	}
}

void SThesisHudPainter::PaintDamageVignette(FPaintContext& Context, const UThesisGameSubsystem& Owner, const FVector2D& LocalSize) const
{
	const double Remaining = Owner.GetDamageFlashUntil() - Owner.GetRealTime();
	if (Remaining <= 0.0)
	{
		return;
	}

	const float Strength = static_cast<float>(FMath::Clamp(Remaining / 0.45, 0.0, 1.0));
	constexpr int32 Strips = 10;
	constexpr double Thickness = 12.0;
	for (int32 Index = 0; Index < Strips; ++Index)
	{
		const float Alpha = 0.42f * Strength * (1.f - static_cast<float>(Index) / Strips);
		const FLinearColor Color = WithAlpha(ThesisStyle::DangerColor(), Alpha);
		const double Offset = Index * Thickness;
		Box(Context, FVector2D(0.0, Offset), FVector2D(LocalSize.X, Thickness), Color, ThesisStyle::Solid());
		Box(Context, FVector2D(0.0, LocalSize.Y - Offset - Thickness), FVector2D(LocalSize.X, Thickness), Color, ThesisStyle::Solid());
		Box(Context, FVector2D(Offset, 0.0), FVector2D(Thickness, LocalSize.Y), Color, ThesisStyle::Solid());
		Box(Context, FVector2D(LocalSize.X - Offset - Thickness, 0.0), FVector2D(Thickness, LocalSize.Y), Color, ThesisStyle::Solid());
	}
}

void SThesisHudPainter::PaintDebugPanel(FPaintContext& Context, const UThesisGameSubsystem& Owner, const FVector2D& LocalSize) const
{
	const TArray<FThesisEnemy>& Enemies = Owner.GetEnemies();

	TArray<const FThesisEnemy*> Rows;
	for (const FThesisEnemy& Enemy : Enemies)
	{
		Rows.Add(&Enemy);
	}
	Rows.StableSort([](const FThesisEnemy& A, const FThesisEnemy& B) { return A.bAlive && !B.bAlive; });
	if (Rows.Num() > MaxDebugRows)
	{
		Rows.SetNum(MaxDebugRows);
	}

	TArray<FString> PerceptionLines;
	TSet<EThesisEnemyKind> SeenKinds;
	for (const FThesisEnemy& Enemy : Enemies)
	{
		if (Enemy.SightRadius <= 0.f || SeenKinds.Contains(Enemy.Kind))
		{
			continue;
		}
		SeenKinds.Add(Enemy.Kind);
		FString Line = FString::Printf(TEXT("%s: wzrok %s, kąt ±%s°"), *ThesisText::KindLabel(Enemy.Kind), *ThesisText::Meters(Enemy.SightRadius), *ThesisText::Number(Enemy.SightHalfAngle, 0));
		if (Enemy.HearingRange > 0.f)
		{
			Line += FString::Printf(TEXT(", słuch %s"), *ThesisText::Meters(Enemy.HearingRange));
		}
		PerceptionLines.Add(Line);
	}

	const TArray<FThesisLogLine>& Log = Owner.GetDecisionLog();
	const int32 LogLines = FMath::Min(Log.Num(), 8);

	const double Height = 18.0 + 24.0 + 22.0 + 20.0 + PerceptionLines.Num() * 18.0 + 12.0 + 22.0 + FMath::Max(1, Rows.Num()) * 22.0 + 14.0 + 22.0 + FMath::Max(1, LogLines) * 19.0 + 14.0 + 20.0 + 14.0;
	const FVector2D Position(LocalSize.X - Margin - DebugWidth, Margin + StatsHeight() + 12.0);
	Box(Context, Position, FVector2D(DebugWidth, Height), ThesisStyle::PanelStrongColor(), ThesisStyle::Rounded());

	const double Left = Position.X + 18.0;
	double Y = Position.Y + 14.0;

	const FVector2D TitleSize = Text(Context, TEXT("PANEL AI"), FVector2D(Left, Y), ThesisStyle::Font(14, true), ThesisStyle::AccentColor());
	Text(Context, TEXT("podgląd decyzji drzewa zachowań"), FVector2D(Left + TitleSize.X + 10.0, Y + 3.0), ThesisStyle::Font(11, false), ThesisStyle::TextDimColor());
	Y += 24.0;

	const double Reaction = Owner.GetAverageReaction();
	const FString Summary = FString::Printf(TEXT("FPS %s   ·   aktywni %d / %d   ·   zmiany zachowań %d   ·   oscylacje %d   ·   śr. reakcja %s"),
		*ThesisText::Number(Owner.GetFps(), 0),
		Owner.GetHostileAlive(), Owner.GetHostileTotal(),
		Owner.GetTotalTransitions(),
		Owner.GetTotalOscillations(),
		Reaction >= 0.0 ? *ThesisText::Seconds(Reaction, 2) : TEXT("—"));
	Text(Context, Summary, FVector2D(Left, Y), ThesisStyle::Font(11, false), ThesisStyle::TextColor());
	Y += 22.0;

	Text(Context, TEXT("PERCEPCJA (odczyt z AIPerception kontrolera)"), FVector2D(Left, Y), ThesisStyle::Font(10, true), ThesisStyle::TextDimColor());
	Y += 20.0;
	for (const FString& Line : PerceptionLines)
	{
		Text(Context, Line, FVector2D(Left, Y), ThesisStyle::Font(11, false), ThesisStyle::TextColor());
		Y += 18.0;
	}
	Y += 12.0;

	const double Columns[] = { 0.0, 150.0, 330.0, 395.0, 480.0, 530.0 };
	const TCHAR* Headers[] = { TEXT("PRZECIWNIK"), TEXT("ZACHOWANIE (BB: Behavior)"), TEXT("HP"), TEXT("DYSTANS"), TEXT("CEL"), TEXT("W STANIE") };
	for (int32 Column = 0; Column < static_cast<int32>(UE_ARRAY_COUNT(Headers)); ++Column)
	{
		Text(Context, Headers[Column], FVector2D(Left + Columns[Column], Y), ThesisStyle::Font(9, true), ThesisStyle::TextDimColor());
	}
	Y += 22.0;

	if (Rows.Num() == 0)
	{
		Text(Context, TEXT("Brak wykrytych przeciwników"), FVector2D(Left, Y), ThesisStyle::Font(11, false), ThesisStyle::TextDimColor());
		Y += 22.0;
	}

	const FSlateFontInfo RowFont = ThesisStyle::Font(11, false);
	const FSlateFontInfo ChipFont = ThesisStyle::Font(10, true);
	for (const FThesisEnemy* Enemy : Rows)
	{
		const float Alpha = Enemy->bAlive ? 1.f : 0.45f;
		const FLinearColor RowColor = WithAlpha(ThesisStyle::TextColor(), Alpha);
		Text(Context, Enemy->DisplayName + (Enemy->bHostile ? FString() : FString(TEXT(" *"))), FVector2D(Left + Columns[0], Y), RowFont, RowColor);

		const FString BehaviorId = Enemy->bAlive ? Enemy->BehaviorId : FString(TEXT("Dead"));
		const FString Label = ThesisText::BehaviorLabel(BehaviorId);
		const FVector2D LabelSize = Measure(Label, ChipFont);
		Box(Context, FVector2D(Left + Columns[1], Y - 1.0), FVector2D(LabelSize.X + 14.0, 19.0), WithAlpha(ThesisStyle::BehaviorColor(ThesisText::BehaviorGroup(BehaviorId)), 0.9f * Alpha), ThesisStyle::Pill());
		Text(Context, Label, FVector2D(Left + Columns[1] + 7.0, Y + 8.5 - LabelSize.Y * 0.5), ChipFont, RowColor, false);

		Text(Context, ThesisText::Number(Enemy->Health, 0), FVector2D(Left + Columns[2], Y), RowFont, RowColor);
		Text(Context, ThesisText::Meters(Enemy->LastDistanceToPlayer), FVector2D(Left + Columns[3], Y), RowFont, RowColor);
		Text(Context, Enemy->bTargetsPlayer ? TEXT("gracz") : TEXT("—"), FVector2D(Left + Columns[4], Y), RowFont, Enemy->bTargetsPlayer ? WithAlpha(ThesisStyle::DangerColor(), Alpha) : RowColor);
		const FString InState = Enemy->bAlive ? ThesisText::Seconds(Owner.GetNow() - Enemy->BehaviorSince, 1) : FString(TEXT("—"));
		Text(Context, InState, FVector2D(Left + Columns[5], Y), RowFont, RowColor);
		Y += 22.0;
	}
	Y += 14.0;

	Text(Context, TEXT("DZIENNIK DECYZJI"), FVector2D(Left, Y), ThesisStyle::Font(10, true), ThesisStyle::TextDimColor());
	Y += 22.0;
	if (LogLines == 0)
	{
		Text(Context, TEXT("Brak zmian zachowań"), FVector2D(Left, Y), ThesisStyle::Font(11, false), ThesisStyle::TextDimColor());
		Y += 19.0;
	}
	for (int32 Index = 0; Index < LogLines; ++Index)
	{
		const FThesisLogLine& Line = Log[Index];
		Box(Context, FVector2D(Left, Y + 5.0), FVector2D(8.0, 8.0), ThesisStyle::BehaviorColor(Line.Group), ThesisStyle::Pill());
		const FVector2D TimeSize = Text(Context, ThesisText::Clock(Line.Time), FVector2D(Left + 16.0, Y), ThesisStyle::Font(11, true), ThesisStyle::TextDimColor());
		Text(Context, Line.Text, FVector2D(Left + 16.0 + TimeSize.X + 10.0, Y), ThesisStyle::Font(11, false), ThesisStyle::TextColor());
		Y += 19.0;
	}
	Y += 14.0;

	FString Footer = TEXT("Telemetria: zapis po zakończeniu gry lub klawiszem F3 → Saved/TelemetriaAI");
	if (!Owner.GetTelemetryFolder().IsEmpty())
	{
		Footer = TEXT("Ostatni zapis: ") + FPaths::GetCleanFilename(Owner.GetTelemetryFolder()) + TEXT("  (Saved/TelemetriaAI)");
	}
	Text(Context, Footer, FVector2D(Left, Y), ThesisStyle::Font(10, false), ThesisStyle::TextDimColor());
}
