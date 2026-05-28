using UnrealBuildTool;

public class ApparatusRuntime : ModuleRules
{
    public ApparatusRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new[]
        {
            "Core",
            "CoreUObject",
            "Engine",
        });
    }
}
