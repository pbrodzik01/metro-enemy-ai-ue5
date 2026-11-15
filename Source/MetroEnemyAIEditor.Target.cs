using UnrealBuildTool;

public class MetroEnemyAIEditorTarget : TargetRules
{
	public MetroEnemyAIEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("MetroEnemyAI");
	}
}
