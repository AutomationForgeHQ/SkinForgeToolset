using UnrealBuildTool;

public class SkinForgeToolset : ModuleRules
{
	public SkinForgeToolset(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"ToolsetRegistry",  // UToolsetDefinition is a public base class
			"SkinForge"         // its structs are the tools' parameter and return types
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Projects",         // IPluginManager, so GetToolsetVersion() reads the descriptor
			"Json",             // Edit Skin Layer takes and returns a layer as JSON
			"JsonUtilities",
			"UnrealEd"          // saving what Apply Skin Version changed, when asked to
		});

		// No dependency on ModelContextProtocol: tools register with ToolsetRegistry and MCP reads them from there.
	}
}
