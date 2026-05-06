using UnrealBuildTool;

public class AMigaXenon : ModuleRules
{
    public AMigaXenon(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "RetroScreen"
            }
        );
    }
}
