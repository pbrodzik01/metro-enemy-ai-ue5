#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ThesisTypes.h"
#include "ThesisSessionSubsystem.generated.h"

UCLASS()
class METROENEMYAI_API UThesisSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	bool ConsumeSkipMenu();
	void RequestSkipMenuOnNextLoad() { bSkipMenuOnNextLoad = true; }

	EThesisLabelMode LabelMode = EThesisLabelMode::All;
	bool bDebugPanelVisible = false;

private:
	bool bSkipMenuOnNextLoad = false;
};
