#include "UI/SThesisMenu.h"

#include "Misc/Paths.h"
#include "ThesisGameSubsystem.h"
#include "ThesisText.h"
#include "UI/ThesisStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	FText Txt(const TCHAR* Value)
	{
		return FText::FromString(Value);
	}

	TSharedRef<SWidget> Gap(float Height)
	{
		return SNew(SSpacer).Size(FVector2D(1.f, Height));
	}
}

void SThesisMenu::Construct(const FArguments& InArgs)
{
	Subsystem = InArgs._Subsystem;
	SetVisibility(TAttribute<EVisibility>::CreateSP(this, &SThesisMenu::GetMenuVisibility));

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(ThesisStyle::Solid())
		.BorderBackgroundColor(this, &SThesisMenu::GetBackdropColor)
		.Padding(0.f)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SWidgetSwitcher)
			.WidgetIndex(this, &SThesisMenu::GetPageIndex)
			+ SWidgetSwitcher::Slot()
			[
				MakeMainPage()
			]
			+ SWidgetSwitcher::Slot()
			[
				MakeControlsPage()
			]
			+ SWidgetSwitcher::Slot()
			[
				MakePausePage()
			]
			+ SWidgetSwitcher::Slot()
			[
				MakeEndPage()
			]
		]
	];
}

EVisibility SThesisMenu::GetMenuVisibility() const
{
	const UThesisGameSubsystem* Owner = Subsystem.Get();
	if (!Owner || Owner->GetGameState() == EThesisGameState::Playing)
	{
		return EVisibility::Collapsed;
	}
	return Owner->IsAutomationActive() ? EVisibility::HitTestInvisible : EVisibility::Visible;
}

int32 SThesisMenu::GetPageIndex() const
{
	const UThesisGameSubsystem* Owner = Subsystem.Get();
	if (!Owner)
	{
		return 0;
	}

	switch (Owner->GetGameState())
	{
	case EThesisGameState::Controls: return 1;
	case EThesisGameState::Paused: return 2;
	case EThesisGameState::Won:
	case EThesisGameState::Lost: return 3;
	default: return 0;
	}
}

FSlateColor SThesisMenu::GetBackdropColor() const
{
	const UThesisGameSubsystem* Owner = Subsystem.Get();
	const bool bBeforeStart = !Owner || !Owner->HasSessionStarted();
	return FLinearColor(0.008f, 0.010f, 0.014f, bBeforeStart ? 0.9f : 0.62f);
}

FReply SThesisMenu::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	UThesisGameSubsystem* Owner = Subsystem.Get();
	if (!Owner)
	{
		return FReply::Unhandled();
	}

	const FKey Key = InKeyEvent.GetKey();
	const bool bBack = Key == EKeys::Escape || Key == EKeys::BackSpace;
	switch (Owner->GetGameState())
	{
	case EThesisGameState::Paused:
		if (bBack || Key == EKeys::P)
		{
			Owner->Resume();
			return FReply::Handled();
		}
		break;
	case EThesisGameState::Controls:
		if (bBack)
		{
			Owner->CloseControls();
			return FReply::Handled();
		}
		break;
	case EThesisGameState::MainMenu:
		if (Key == EKeys::Enter)
		{
			Owner->StartGame();
			return FReply::Handled();
		}
		break;
	case EThesisGameState::Won:
	case EThesisGameState::Lost:
		if (Key == EKeys::Enter)
		{
			Owner->RestartLevel(false);
			return FReply::Handled();
		}
		break;
	default:
		break;
	}
	return FReply::Unhandled();
}

TSharedRef<SWidget> SThesisMenu::MakePanel(const TSharedRef<SWidget>& Content, float Width) const
{
	return SNew(SBox)
		.WidthOverride(Width)
		[
			SNew(SBorder)
			.BorderImage(ThesisStyle::Rounded())
			.BorderBackgroundColor(ThesisStyle::PanelStrongColor())
			.Padding(FMargin(40.f, 34.f))
			[
				Content
			]
		];
}

TSharedRef<SWidget> SThesisMenu::MakeTitle(const TAttribute<FText>& Title, const TAttribute<FText>& Subtitle, const TAttribute<FSlateColor>& TitleColor) const
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Left)
		[
			SNew(SBox)
			.WidthOverride(44.f)
			.HeightOverride(4.f)
			[
				SNew(SBorder)
				.BorderImage(ThesisStyle::RoundedSmall())
				.BorderBackgroundColor(TitleColor)
			]
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 14.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text(Title)
			.Font(ThesisStyle::Font(26, true))
			.ColorAndOpacity(ThesisStyle::TextColor())
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 8.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text(Subtitle)
			.Font(ThesisStyle::Font(12, false))
			.ColorAndOpacity(ThesisStyle::TextDimColor())
			.AutoWrapText(true)
		];
}

TSharedRef<SWidget> SThesisMenu::MakeButton(const FText& Label, TFunction<void(UThesisGameSubsystem&)> Action, bool bPrimary)
{
	const TWeakObjectPtr<UThesisGameSubsystem> WeakOwner = Subsystem;
	return SNew(SBox)
		.HeightOverride(48.f)
		[
			SNew(SButton)
			.ButtonStyle(&ThesisStyle::Button(bPrimary))
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.OnClicked_Lambda([WeakOwner, Action]()
			{
				if (UThesisGameSubsystem* Owner = WeakOwner.Get())
				{
					Action(*Owner);
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(Label)
				.Font(ThesisStyle::Font(15, true))
				.ColorAndOpacity(bPrimary ? FLinearColor(0.02f, 0.02f, 0.02f, 1.f) : ThesisStyle::TextColor())
			]
		];
}

TSharedRef<SWidget> SThesisMenu::MakeKeyRow(const FText& Key, const FText& Description) const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.MinDesiredWidth(120.f)
			[
				SNew(SBorder)
				.BorderImage(ThesisStyle::RoundedSmall())
				.BorderBackgroundColor(FLinearColor(1.f, 1.f, 1.f, 0.07f))
				.Padding(FMargin(10.f, 5.f))
				.HAlign(HAlign_Center)
				[
					SNew(STextBlock)
					.Text(Key)
					.Font(ThesisStyle::Font(12, true))
					.ColorAndOpacity(ThesisStyle::AccentColor())
				]
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		.Padding(16.f, 0.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text(Description)
			.Font(ThesisStyle::Font(12, false))
			.ColorAndOpacity(ThesisStyle::TextColor())
			.AutoWrapText(true)
		];
}

TSharedRef<SWidget> SThesisMenu::MakeStatRow(TFunction<FText()> Label, TFunction<FText()> Value) const
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda(MoveTemp(Label))
			.Font(ThesisStyle::Font(12, false))
			.ColorAndOpacity(ThesisStyle::TextDimColor())
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text_Lambda(MoveTemp(Value))
			.Font(ThesisStyle::Font(14, true))
			.ColorAndOpacity(ThesisStyle::TextColor())
		];
}

TSharedRef<SWidget> SThesisMenu::MakeMainPage()
{
	return MakePanel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeTitle(
				Txt(TEXT("Prototyp AI przeciwników")),
				Txt(TEXT("Środowisko badawczo-testowe pracy inżynierskiej. Przeciwnicy sterowani drzewem zachowań (Behavior Tree), Blackboardem i systemem AI Perception w Unreal Engine 5.6.")),
				ThesisStyle::AccentColor())
		]
		+ SVerticalBox::Slot().AutoHeight()[Gap(28.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Rozpocznij")), [](UThesisGameSubsystem& Owner) { Owner.StartGame(); }, true)]
		+ SVerticalBox::Slot().AutoHeight()[Gap(10.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Sterowanie")), [](UThesisGameSubsystem& Owner) { Owner.ShowControls(); }, false)]
		+ SVerticalBox::Slot().AutoHeight()[Gap(10.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Wyjście")), [](UThesisGameSubsystem& Owner) { Owner.QuitGame(); }, false)]
		+ SVerticalBox::Slot().AutoHeight()[Gap(24.f)]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(Txt(TEXT("W grze: F1 panel AI · F2 etykiety · F5 test obciążenia · P pauza.\nPrzebieg decyzji AI zapisuje się do plików CSV w Saved/TelemetriaAI.")))
			.Font(ThesisStyle::Font(11, false))
			.ColorAndOpacity(ThesisStyle::TextDimColor())
			.AutoWrapText(true)
		],
		520.f);
}

TSharedRef<SWidget> SThesisMenu::MakeControlsPage()
{
	struct FKeyEntry
	{
		const TCHAR* Key;
		const TCHAR* Description;
	};
	const FKeyEntry Entries[] = {
		{ TEXT("W S A D"), TEXT("ruch postaci") },
		{ TEXT("Mysz"), TEXT("obrót kamery i celowanie") },
		{ TEXT("LPM"), TEXT("strzał") },
		{ TEXT("Spacja"), TEXT("skok") },
		{ TEXT("P / Esc"), TEXT("pauza (w edytorze Esc kończy podgląd, używaj P)") },
		{ TEXT("F1"), TEXT("panel AI: zachowanie z Blackboardu, cel, dystans, dziennik decyzji") },
		{ TEXT("F2"), TEXT("etykiety nad przeciwnikami: wszystkie / tylko widoczne / wyłączone") },
		{ TEXT("F3"), TEXT("zapis telemetrii do CSV") },
		{ TEXT("F5"), TEXT("test obciążenia: +5 przeciwników w pobliżu gracza") }
	};

	TSharedRef<SVerticalBox> List = SNew(SVerticalBox);
	for (const FKeyEntry& Entry : Entries)
	{
		List->AddSlot().AutoHeight().Padding(0.f, 5.f)[MakeKeyRow(Txt(Entry.Key), Txt(Entry.Description))];
	}

	return MakePanel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeTitle(Txt(TEXT("Sterowanie")), Txt(TEXT("Klawisze F działają w trakcie rozgrywki.")), ThesisStyle::AccentColor())
		]
		+ SVerticalBox::Slot().AutoHeight()[Gap(20.f)]
		+ SVerticalBox::Slot().AutoHeight()[List]
		+ SVerticalBox::Slot().AutoHeight()[Gap(24.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Wróć")), [](UThesisGameSubsystem& Owner) { Owner.CloseControls(); }, true)],
		600.f);
}

TSharedRef<SWidget> SThesisMenu::MakePausePage()
{
	const TWeakObjectPtr<UThesisGameSubsystem> WeakOwner = Subsystem;

	return MakePanel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			MakeTitle(Txt(TEXT("Pauza")), Txt(TEXT("Rozgrywka, AI i fizyka są wstrzymane.")), ThesisStyle::AccentColor())
		]
		+ SVerticalBox::Slot().AutoHeight()[Gap(18.f)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f)
		[
			MakeStatRow([]() { return Txt(TEXT("Czas rozgrywki")); }, [WeakOwner]() { return FText::FromString(WeakOwner.IsValid() ? ThesisText::Clock(WeakOwner->GetSessionTime()) : FString()); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f)
		[
			MakeStatRow([]() { return Txt(TEXT("Pozostali wrogowie")); }, [WeakOwner]() { return FText::FromString(WeakOwner.IsValid() ? FString::Printf(TEXT("%d / %d"), WeakOwner->GetHostileAlive(), WeakOwner->GetHostileTotal()) : FString()); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f)
		[
			MakeStatRow([]() { return Txt(TEXT("Zmiany zachowań AI")); }, [WeakOwner]() { return FText::AsNumber(WeakOwner.IsValid() ? WeakOwner->GetTotalTransitions() : 0); })
		]
		+ SVerticalBox::Slot().AutoHeight()[Gap(22.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Wznów")), [](UThesisGameSubsystem& Owner) { Owner.Resume(); }, true)]
		+ SVerticalBox::Slot().AutoHeight()[Gap(10.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Sterowanie")), [](UThesisGameSubsystem& Owner) { Owner.ShowControls(); }, false)]
		+ SVerticalBox::Slot().AutoHeight()[Gap(10.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Zacznij od nowa")), [](UThesisGameSubsystem& Owner) { Owner.RestartLevel(false); }, false)]
		+ SVerticalBox::Slot().AutoHeight()[Gap(10.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Menu główne")), [](UThesisGameSubsystem& Owner) { Owner.RestartLevel(true); }, false)]
		+ SVerticalBox::Slot().AutoHeight()[Gap(10.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Wyjście")), [](UThesisGameSubsystem& Owner) { Owner.QuitGame(); }, false)],
		460.f);
}

TSharedRef<SWidget> SThesisMenu::MakeEndPage()
{
	const TWeakObjectPtr<UThesisGameSubsystem> WeakOwner = Subsystem;
	const auto IsWon = [WeakOwner]() { return WeakOwner.IsValid() && WeakOwner->GetGameState() == EThesisGameState::Won; };

	const TAttribute<FText> Title = TAttribute<FText>::CreateLambda([IsWon]() { return IsWon() ? Txt(TEXT("Zwycięstwo")) : Txt(TEXT("Porażka")); });
	const TAttribute<FText> Subtitle = TAttribute<FText>::CreateLambda([IsWon]()
	{
		return IsWon()
			? Txt(TEXT("Wszyscy wrogowie na stacji zostali wyeliminowani."))
			: Txt(TEXT("Postać gracza została wyeliminowana."));
	});
	const TAttribute<FSlateColor> TitleColor = TAttribute<FSlateColor>::CreateLambda([IsWon]() { return FSlateColor(IsWon() ? ThesisStyle::GoodColor() : ThesisStyle::DangerColor()); });

	return MakePanel(
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[MakeTitle(Title, Subtitle, TitleColor)]
		+ SVerticalBox::Slot().AutoHeight()[Gap(18.f)]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f)
		[
			MakeStatRow([]() { return Txt(TEXT("Czas rozgrywki")); }, [WeakOwner]() { return FText::FromString(WeakOwner.IsValid() ? ThesisText::Clock(WeakOwner->GetSessionTime()) : FString()); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f)
		[
			MakeStatRow([]() { return Txt(TEXT("Eliminacje")); }, [WeakOwner]() { return FText::FromString(WeakOwner.IsValid() ? FString::Printf(TEXT("%d / %d"), WeakOwner->GetKills(), WeakOwner->GetHostileTotal()) : FString()); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f)
		[
			MakeStatRow([]() { return Txt(TEXT("Zmiany zachowań AI")); }, [WeakOwner]() { return FText::AsNumber(WeakOwner.IsValid() ? WeakOwner->GetTotalTransitions() : 0); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f)
		[
			MakeStatRow([]() { return Txt(TEXT("Oscylacje gałęzi (< 1 s)")); }, [WeakOwner]() { return FText::AsNumber(WeakOwner.IsValid() ? WeakOwner->GetTotalOscillations() : 0); })
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f)
		[
			MakeStatRow([]() { return Txt(TEXT("Średni czas reakcji AI")); }, [WeakOwner]()
			{
				const double Reaction = WeakOwner.IsValid() ? WeakOwner->GetAverageReaction() : -1.0;
				return FText::FromString(Reaction >= 0.0 ? ThesisText::Seconds(Reaction, 2) : FString(TEXT("brak pomiaru")));
			})
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 14.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text_Lambda([WeakOwner]()
			{
				const FString Folder = WeakOwner.IsValid() ? WeakOwner->GetTelemetryFolder() : FString();
				return FText::FromString(Folder.IsEmpty()
					? FString(TEXT("Telemetria nie została zapisana."))
					: FString::Printf(TEXT("Telemetria zapisana w Saved/TelemetriaAI/%s"), *FPaths::GetCleanFilename(Folder)));
			})
			.Font(ThesisStyle::Font(11, false))
			.ColorAndOpacity(ThesisStyle::TextDimColor())
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight()[Gap(22.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Zagraj ponownie")), [](UThesisGameSubsystem& Owner) { Owner.RestartLevel(false); }, true)]
		+ SVerticalBox::Slot().AutoHeight()[Gap(10.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Menu główne")), [](UThesisGameSubsystem& Owner) { Owner.RestartLevel(true); }, false)]
		+ SVerticalBox::Slot().AutoHeight()[Gap(10.f)]
		+ SVerticalBox::Slot().AutoHeight()[MakeButton(Txt(TEXT("Wyjście")), [](UThesisGameSubsystem& Owner) { Owner.QuitGame(); }, false)],
		480.f);
}
