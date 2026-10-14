using UnrealBuildTool;
using System.Collections.Generic;

public class TransoxianaEditorTarget : TargetRules
{
	public TransoxianaEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Transoxiana");
	}
}
