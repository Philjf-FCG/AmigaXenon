using UnrealBuildTool;

public class RetroScreen : ModuleRules
{
    public RetroScreen(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        IWYUSupport = IWYUSupport.Full;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "EnhancedInput",
                "RenderCore",
                "RHI",
                "Projects"
            }
        );

        // UnrealLibretro is public because RetroScreenLibretroCore.h (a public header)
        // uses libretro types directly, so consumers need UnrealLibretro's include paths.
        PublicDependencyModuleNames.Add("UnrealLibretro");

        // UMG must be public: RetroScreenRemapWidget.h (a public header) includes Blueprint/UserWidget.h
        PublicDependencyModuleNames.Add("UMG");

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "Slate",
                "SlateCore",
                "AudioMixer",
            }
        );
    }
}
