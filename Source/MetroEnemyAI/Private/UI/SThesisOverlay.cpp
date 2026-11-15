#include "UI/SThesisOverlay.h"

#include "ThesisGameSubsystem.h"
#include "UI/SThesisHudPainter.h"
#include "UI/SThesisMenu.h"
#include "Widgets/SOverlay.h"

void SThesisOverlay::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SThesisHudPainter)
			.Subsystem(InArgs._Subsystem)
			.Visibility(EVisibility::HitTestInvisible)
		]
		+ SOverlay::Slot()
		[
			SAssignNew(Menu, SThesisMenu)
			.Subsystem(InArgs._Subsystem)
		]
	];
}

TSharedPtr<SWidget> SThesisOverlay::GetMenuFocusTarget() const
{
	return Menu;
}
