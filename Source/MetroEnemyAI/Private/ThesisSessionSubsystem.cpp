#include "ThesisSessionSubsystem.h"

bool UThesisSessionSubsystem::ConsumeSkipMenu()
{
	const bool bSkip = bSkipMenuOnNextLoad;
	bSkipMenuOnNextLoad = false;
	return bSkip;
}
