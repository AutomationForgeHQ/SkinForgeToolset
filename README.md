<!-- kovati:header -->
<p align="center"><a href="https://kovati.dev/tools/skinforge/"><img src="https://raw.githubusercontent.com/AutomationForgeHQ/.github/main/profile/assets/skinforge.jpg" width="100%" alt="SkinForge, part of Kovati" /></a></p>

<p align="center"><b>Agent toolset for SkinForge</b> · Part of <a href="https://kovati.dev"><b>Kovati</b></a>, production tools for Unreal games · <a href="https://kovati.dev/characters/">Characters</a><br />
<a href="https://kovati.dev/tools/skinforge/">Product page</a> · <a href="https://docs.kovati.dev">Documentation</a> · <a href="https://kovati.dev/download/">Install with the Kovati Hub</a> · <a href="https://kovati.dev/discord">Discord</a></p>
<!-- /kovati:header -->

# SkinForge Toolset

<!-- forge:version -->**Version 0.3.1. Experimental.**<!-- /forge:version --> Free and open source, Apache-2.0.

SkinForge as Model Context Protocol tools, through Unreal Engine 5.8's toolset registry. An MCP client can do what a
person does in the SkinForge panel: make a skin set over a character's meshes, stamp marks on it, change their look,
bake a version, see the result, and put the version on the character's assets and actors.

The toolset is an adapter and nothing more. Every tool calls the same SkinForge function the panel calls, so an agent
and a person can work on the same set.

## Requirements

- Unreal Engine 5.8, editor only.
- **SkinForge 0.4.0 or later**, the plugin this toolset drives. SkinForge is a paid product with a free Lite tier,
  installed through the Kovati Hub. The toolset has no tiers of its own: in Lite, SkinForge bakes every map at most
  2K, and the bake tools return its note saying so. See SkinForge's README for Lite and Full.
- The engine's **ToolsetRegistry** plugin, which this plugin enables. To reach the tools from an MCP client, also
  enable the engine's **Model Context Protocol** plugin.

Disabled by default, like every toolset: enable **SkinForgeToolset** in **Edit → Plugins**.

## The tools

<!-- forge:tools -->24 tools<!-- /forge:tools -->. Each row is taken from the function's own doc comment, which is also
the description an agent reads.

| Area | Tool | What it does |
| --- | --- | --- |
| Look | `DescribeSkinTarget` | Material slots, the LODs that draw them, every texture parameter (size, colour space, compression, source format), bounds and UV range. Call it first. |
| | `TraceSkinSurface` | Cast a ray against LOD 0 in the reference pose; the hit is a projector's centre and normal. |
| | `ProbeTextureChannels` | Which channel of a packed map feeds what in a material graph. |
| | `CaptureSkinPreview` | Offscreen before and after pictures; touches no level. |
| One patch | `StampSkinPatch` | Project one patch and bake it into copies of the textures and child material instances, in a new dated folder. |
| Sets | `CreateSkinSet` | A new skin set over some skeletal meshes (for a MetaHuman, its face and body). |
| | `AddStarterSkinDecals` | Make the starter library that ships with SkinForge into decal assets. |
| | `ImportSkinMarks` | Make a folder of mark images and its `starter_decals.json` into mark assets: your own library. |
| | `DescribeSkinMark` | A mark's stages, ink slots, controls and presets. |
| Layers | `StampSkinLayer` | Stamp a new layer where a ray meets the set's meshes, as a click in the panel does. |
| | `EditSkinLayer` | Change some properties of one layer by name, including a mark's controls, inks and presets. |
| Versions | `BakeSkinSet` | Bake every visible layer into a new version; earlier versions stay. |
| | `ListSkinVersions` | A set's versions and which apply targets wear each. |
| | `RestoreSkinVersionLayers` | Bring a version's layers back, to change them and bake again. |
| | `IsSkinSetRenderReady` | Whether a version's shaders, textures and glow maps are ready to photograph. |
| Apply | `FindSkinMaterialUsers` | The assets that use the materials a set replaces. |
| | `ApplySkinVersion` | Point a target's references to the originals at a version's materials; an empty version reverts. Undoable. |
| | `DescribeSkinApply` | Which version each apply target wears now. |
| Panel | `OpenSkinForge` | Open the SkinForge panel, optionally on a set. |
| | `SetSkinForgeView` | Show Original, Projection or Baked in the panel's viewport. |
| | `AimSkinForgeCamera` | Point the panel's camera at a point. |
| | `CaptureSkinForgePanel` | Write the panel's viewport to a PNG. |
| | `ClickSkinForgeViewport` | A left click in the panel's viewport, with Shift or Alt if asked. |
| | `ArmSkinDecal` | Pick a mark in the panel's library, and set the stamp mode. |

Parameters and results are reflected structs, so the schema an agent reads is the signature.

There is no agent skill yet: the tools' descriptions are what an agent reads.

## Deliberately absent

- **No tool edits an original texture or material.** Bakes write copies of the textures and child material instances, in a folder of
  their own; Apply changes only material references, and Revert puts the originals back.
- **No tool deletes a version.** Deleting one is a person's choice, in the panel.

## Licence

Apache-2.0, in [LICENSE](LICENSE). SkinForge itself is a paid product, with a free Lite tier, under its own licence.
