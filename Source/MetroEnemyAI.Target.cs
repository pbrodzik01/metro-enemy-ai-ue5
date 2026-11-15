using UnrealBuildTool;

public class MetroEnemyAITarget : TargetRules
{
	public MetroEnemyAITarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MetroEnemyAI");
	}
}
