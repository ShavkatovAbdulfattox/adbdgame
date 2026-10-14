using UnrealBuildTool;

public class Transoxiana : ModuleRules
{
	public Transoxiana(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets headers include each other as "Player/TransoxianaCharacter.h" etc.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"AudioMixer",
			"UMG",
			"Slate",
			"SlateCore"
		});
	}
}
