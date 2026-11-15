#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UThesisGameSubsystem;
class SThesisMenu;

class SThesisOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SThesisOverlay) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UThesisGameSubsystem>, Subsystem)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	TSharedPtr<SWidget> GetMenuFocusTarget() const;

private:
	TSharedPtr<SThesisMenu> Menu;
};
