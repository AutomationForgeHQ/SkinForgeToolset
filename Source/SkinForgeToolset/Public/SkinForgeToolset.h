#pragma once

#include "CoreMinimal.h"
#include "SkinForgeTypes.h"
#include "ToolsetRegistry/ToolsetDefinition.h"
#include "SkinForgeToolset.generated.h"

/**
 * Stamp scars, tattoos and other marks onto any skeletal mesh, baked into copies of the textures its
 * materials already use, with child material instances that swap the copies in.
 *
 * Start with Describe Skin Target to see which texture parameters each slot has. Trace Skin Surface finds a
 * point and normal on the skin for the projector, and Stamp Skin Patch bakes. Capture Skin Preview shows the
 * result offscreen next to the original. Each baked map also comes with a PNG of its stamped area in UV space.
 *
 * Bakes are versions. Apply Skin Version puts one where the original materials are used - an appearance, a
 * Blueprint, the meshes, level actors - by changing only their material references, and an empty version
 * puts the originals back. Nothing else edits an asset it was given: bakes write new assets only.
 */
UCLASS(BlueprintType)
class SKINFORGETOOLSET_API USkinForgeToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	virtual FString GetToolsetVersion() const override;

	/**
	 * What SkinForge sees in each mesh: material slots, which LODs draw them, every texture parameter with
	 * its size, colour space, compression and source format, LOD 0 bounds, and the UV range. Call it first:
	 * the parameter names it lists are what Stamp Skin Patch matches.
	 *
	 * @param Meshes Skeletal mesh asset paths.
	 * @return One entry per mesh.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Discovery")
	static TArray<FSkinForgeMeshInfo> DescribeSkinTarget(const TArray<FString>& Meshes);

	/**
	 * Casts a ray against LOD 0 of the meshes in their reference pose and returns the nearest hit with its
	 * smooth normal and UV. Use the hit's Location and Normal as a stamp projector's Center and Normal.
	 *
	 * @param Meshes Skeletal mesh asset paths.
	 * @param Start Where the ray starts, mesh space, centimetres. Start outside the body.
	 * @param Direction The way the ray travels.
	 * @return bHit is false when the ray misses everything.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Discovery")
	static FSkinForgeSurfaceHit TraceSkinSurface(const TArray<FString>& Meshes, FVector Start, FVector Direction);

	/**
	 * Projects one patch onto the meshes and bakes it into copies of the textures their materials sample
	 * (colour, and relief into the normal maps), then makes child material instances that use the copies.
	 * Nothing given is edited; every run writes a new dated folder. Slow on large textures: an 8k map takes
	 * tens of seconds.
	 *
	 * @param Request Meshes, projector, look and which texture parameters hold colour and normals.
	 * @return The baked maps and materials, and MaterialSwaps for Capture Skin Preview.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Stamp")
	static FSkinForgeStampResult StampSkinPatch(const FSkinForgeStampRequest& Request);

	/**
	 * Renders the meshes offscreen in their reference pose and writes one PNG per look. It touches no level,
	 * so it is safe while someone works in the editor. Capture once without MaterialSwaps and once with a
	 * stamp's MaterialSwaps to compare before and after.
	 *
	 * @param Request Meshes, swaps, camera target and direction, width, looks.
	 * @return Absolute paths of the PNGs, in the order of Looks.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Look")
	static TArray<FString> CaptureSkinPreview(const FSkinForgeCaptureRequest& Request);

	/**
	 * Where each channel of a texture parameter goes in a material graph, following material functions:
	 * which channel of a packed map is roughness, for instance.
	 *
	 * @param Material A material or material instance path; its base material's graph is read.
	 * @param Parameter The texture parameter name, as Describe Skin Target lists it.
	 * @return One entry per connection: sampler output, the node it feeds, and that node's input.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Discovery")
	static TArray<FSkinForgeChannelUse> ProbeTextureChannels(const FString& Material, const FString& Parameter);

	/**
	 * Opens the SkinForge panel - the same window a person uses - optionally on a skin set.
	 *
	 * @param Set A skin set asset path, or empty to open the panel as it is.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static void OpenSkinForge(const FString& Set);

	/**
	 * A new skin set over some skeletal meshes, saved under /Game/_Generated/SkinForge/Sets.
	 *
	 * @param Name Short name; the asset becomes SFS_<Name>.
	 * @param Meshes Skeletal mesh paths. For a MetaHuman, its face and body meshes.
	 * @return The new set's path.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static FString CreateSkinSet(const FString& Name, const TArray<FString>& Meshes);

	/**
	 * Makes the decals that ship with the plugin (scars fresh and old, tattoos, mechanical ports) into decal
	 * assets under /Game/_Generated/SkinForge/Decals. Ones already there are returned as they are.
	 *
	 * @return Decal asset paths.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static TArray<FString> AddStarterSkinDecals();

	/**
	 * Makes a folder of mark images and its starter_decals.json manifest (format v4, v3 or v2, as the
	 * starter library is written) into mark assets: your own library, or a test set. Marks it made before
	 * from the same folder are rewritten from the files.
	 *
	 * @param Directory An absolute folder on disk holding starter_decals.json and the PNGs it names.
	 * @param ContentFolder Where the marks go; it must be under /Game/_Generated/.
	 * @return The marks' asset paths.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static TArray<FString> ImportSkinMarks(const FString& Directory, const FString& ContentFolder);

	/**
	 * Stamps a new layer of a decal where a ray meets the set's meshes, exactly as a click in the panel does,
	 * with the decal's default size and look.
	 *
	 * @param Set Skin set path.
	 * @param Decal Decal asset path (Add Starter Skin Decals lists some).
	 * @param Start Mesh space, centimetres, outside the body.
	 * @param Direction The way the ray travels.
	 * @return The new layer's index.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static int32 StampSkinLayer(const FString& Set, const FString& Decal, FVector Start, FVector Direction);

	/**
	 * Changes some properties of one layer of a set, by name, and leaves the rest as they are - what the
	 * panel's layer details do. Names are the layer's property names, case not minding: ColourBlend ("FromDecal",
	 * "Over", "Ink", "Tone"), bOverrideInkDepth + InkDepthMm, bOverrideKeepSkinDetail + KeepSkinDetail,
	 * bOverridePoreSmoothing + PoreSmoothing, Age, Seed, Opacity, Tint ({"R":..,"G":..,"B":..,"A":..}),
	 * ColourAmount, ReliefCm, RoughnessStrength, SpecularStrength, FuzzStrength, ScatterStrength, SizeCm,
	 * RotationDeg, bMirror, Name, bVisible, Decal (an asset path).
	 *
	 * Three keys set the mark's own settings (Describe Skin Mark lists them):
	 * - "Preset": a preset's name; it is applied first, so other keys can adjust it.
	 * - "Controls": {"Wear":0.6,"Fresh":1,"Pigment":"Dark"} - a slider 0..1, a toggle 0/1 or true/false, a
	 *   choice by option name or index. Merged into the layer's values; an unknown name is an error.
	 * - "Slots": {"Line":[0.06,0.06,0.07],"Colour":"#9E1410"} - an ink per slot, sRGB 0..1 or hex, or a linear
	 *   {"R":..,"G":..,"B":..}; null puts a slot back to the mark's own ink.
	 *
	 * @param Set Skin set path.
	 * @param Layer The layer's index, as Stamp Skin Layer returned it.
	 * @param Properties A JSON object of property name to new value, e.g. {"ColourBlend":"Ink","bOverrideInkDepth":true,"InkDepthMm":0.4}.
	 * @return The whole layer after the change, as JSON, with ResolvedControls and ResolvedSlots (sRGB hex) added.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static FString EditSkinLayer(const FString& Set, int32 Layer, const FString& Properties);

	/**
	 * Bakes every visible layer of a set into a new version - copies of its meshes' textures and child
	 * material instances in a folder of their own - as the panel's Bake button does. Earlier versions stay.
	 *
	 * @param Set Skin set path.
	 * @param Version The new version's name, unique in the set; empty makes it v1, v2 and on.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Versions")
	static FSkinForgeStampResult BakeSkinSet(const FString& Set, const FString& Version = TEXT(""));

	/**
	 * A set's versions, oldest first: name, when, folder, the material swaps each made, and which of the
	 * set's apply targets wear it now.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Versions")
	static TArray<FSkinForgeVersionInfo> ListSkinVersions(const FString& Set);

	/**
	 * Assets that use the materials a set replaces - character appearances, Blueprints, the meshes
	 * themselves - which are the places Apply Skin Version can put a version. Levels are left out; pass a
	 * level's actors to Apply Skin Version directly.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Apply")
	static TArray<FString> FindSkinMaterialUsers(const FString& Set);

	/**
	 * Puts a version on some targets: every reference they hold to the set's original materials, or to
	 * another of its versions, is pointed at this version's. Only material references change; an empty
	 * Version puts the originals back (revert). Undoable in the editor.
	 *
	 * @param Set Skin set path.
	 * @param Version A version's name, or empty to revert to the originals.
	 * @param Targets Asset or actor paths; they join the set's apply targets. Empty uses the set's targets.
	 * @param bSave Save the changed packages and the set. Off leaves them dirty for a person to check.
	 * @return Every reference changed, the packages changed, and targets left alone with why.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Apply")
	static FSkinForgeApplyReport ApplySkinVersion(const FString& Set, const FString& Version, const TArray<FString>& Targets, bool bSave = false);

	/**
	 * Which version each of a set's apply targets wears, read from the references it holds: "Original", a
	 * version's name, "Mixed", or "Unused" when it holds none of the set's materials.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Apply")
	static TArray<FSkinForgeTargetState> DescribeSkinApply(const FString& Set);

	/**
	 * Whether a version is ready to photograph: its baked materials' shaders compiled, its textures built and
	 * its glow maps streamed in. A set's first glowing bake compiles a copy of the skin material in the
	 * background (minutes for a MetaHuman); until then the editor draws the skin without the glow. Cheap:
	 * poll it every few seconds, in separate calls, after Apply and before a capture.
	 * @param Set Skin set path.
	 * @param Version A version's name; empty for the newest.
	 * @return bReady, and what is still pending.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Apply")
	static FSkinForgeRenderReady IsSkinSetRenderReady(const FString& Set, const FString& Version);

	/** Brings a version's layers back as the set's layers, to change them and bake a new version. */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Versions")
	static void RestoreSkinVersionLayers(const FString& Set, const FString& Version);

	/**
	 * Points the panel's viewport camera at a point.
	 *
	 * @param Target Mesh space point to look at.
	 * @param ViewFrom Direction from the target towards the camera; a layer's normal frames it head on.
	 * @param DistanceCm How far the camera stands from the target.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static void AimSkinForgeCamera(FVector Target, FVector ViewFrom, float DistanceCm = 40.f);

	/**
	 * What the panel's viewport shows: "Original", "Projection" (layers live over the originals) or "Baked"
	 * (the last bake's materials).
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static void SetSkinForgeView(const FString& View);

	/**
	 * Writes the panel's viewport, as it is now, to a PNG and returns its path. Read it to see the result.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static FString CaptureSkinForgePanel();

	/**
	 * A left click in the panel's viewport at pixel X, Y - exactly what a mouse click does there. A click on a
	 * layer selects it, even with a mark picked. With a mark picked, a click on bare skin stamps it; in Stamp
	 * once mode the mark is then put down and the new layer selected. bShift stamps and keeps the mark picked;
	 * bAlt stamps over a layer instead of selecting it. With ToX and ToY it then drags to there, sliding the
	 * layer along the skin. Pixels are those of Capture SkinForge Panel's image.
	 *
	 * @return What happened (stamped, picked, or missed and why), then the panel's state.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static FString ClickSkinForgeViewport(int32 X, int32 Y, int32 ToX = -1, int32 ToY = -1, bool bShift = false, bool bAlt = false);

	/**
	 * Picks a mark in the panel's library, as a click on its card does, and optionally sets the stamp mode.
	 *
	 * @param Decal A mark's asset path, or empty to put the picked one down.
	 * @param StampMode "once" (the default mode: one stamp, then the new layer is selected), "keep" (stays picked
	 *        until put down), or empty to leave the mode as it is. Remembered per person, like the panel's switch.
	 * @return The panel's state: the picked mark, the mode, the selected layer, the view and the live preview.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static FString ArmSkinDecal(const FString& Decal, const FString& StampMode = TEXT(""));

	/**
	 * A mark as JSON: its card name, category, tags, regions, size ceiling, whether it may be mirrored, its
	 * stages along Age, its ink slots with their default inks, its controls (type, default, options, what each
	 * drives) and its presets. These are the names Edit Skin Layer takes under Controls, Slots and Preset.
	 *
	 * @param Decal A mark's asset path.
	 */
	UFUNCTION(meta = (AICallable), Category = "SkinForge|Panel")
	static FString DescribeSkinMark(const FString& Decal);
};
