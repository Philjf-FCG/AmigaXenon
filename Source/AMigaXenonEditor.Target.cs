using UnrealBuildTool;
using System.Collections.Generic;

public class AMigaXenonEditorTarget : TargetRules
{
    public AMigaXenonEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

        ExtraModuleNames.AddRange(new string[]
        {
            "AMigaXenon"
        });
    }
}
