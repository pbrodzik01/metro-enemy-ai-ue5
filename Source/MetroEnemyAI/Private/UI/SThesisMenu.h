#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UThesisGameSubsystem;

class SThesisMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SThesisMenu) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UThesisGameSubsystem>, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	TSharedRef<SWidget> MakePanel(const TSharedRef<SWidget>& Content, float Width) const;
	TSharedRef<SWidget> MakeTitle(const TAttribute<FText>& Title, const TAttribute<FText>& Subtitle, const TAttribute<FSlateColor>& TitleColor) const;
	TSharedRef<SWidget> MakeButton(const FText& Label, TFunction<void(UThesisGameSubsystem&)> Action, bool bPrimary);
	TSharedRef<SWidget> MakeKeyRow(const FText& Key, const FText& Description) const;
	TSharedRef<SWidget> MakeStatRow(TFunction<FText()> Label, TFunction<FText()> Value) const;

	TSharedRef<SWidget> MakeMainPage();
	TSharedRef<SWidget> MakeControlsPage();
	TSharedRef<SWidget> MakePausePage();
	TSharedRef<SWidget> MakeEndPage();

	EVisibility GetMenuVisibility() const;
	int32 GetPageIndex() const;
	FSlateColor GetBackdropColor() const;

	TWeakObjectPtr<UThesisGameSubsystem> Subsystem;
};
