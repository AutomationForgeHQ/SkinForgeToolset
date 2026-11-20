#include "SkinForgeToolset.h"

#include "Dom/JsonObject.h"
#include "Engine/SkeletalMesh.h"
#include "FileHelpers.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Interfaces/IPluginManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/Paths.h"
#include "SkinForge.h"
#include "SkinForgeAssets.h"

FString USkinForgeToolset::GetToolsetVersion() const
{
	// The descriptor is the version; nothing here repeats it.
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT(UE_PLUGIN_NAME));
	return Plugin.IsValid() ? Plugin->GetDescriptor().VersionName : FString();
}

TArray<FSkinForgeMeshInfo> USkinForgeToolset::DescribeSkinTarget(const TArray<FString>& Meshes)
{
	TArray<FSkinForgeMeshInfo> Result;
	if (Meshes.Num() == 0)
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("No meshes. Pass skeletal mesh asset paths, e.g. /Game/MetaHumans/<Name>/Face/SKM_<Name>_FaceMesh."));
		return Result;
	}
	for (const FString& Mesh : Meshes)
	{
		FSkinForgeMeshInfo Info;
		FString Error;
		if (!SkinForge::DescribeMesh(Mesh, Info, Error))
		{
			UKismetSystemLibrary::RaiseScriptError(Error);
			return Result;
		}
		Result.Add(MoveTemp(Info));
	}
	return Result;
}

FSkinForgeSurfaceHit USkinForgeToolset::TraceSkinSurface(const TArray<FString>& Meshes, FVector Start, FVector Direction)
{
	FSkinForgeSurfaceHit Hit;
	FString Error;
	if (!SkinForge::TraceSurface(Meshes, Start, Direction, Hit, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
	return Hit;
}

FSkinForgeStampResult USkinForgeToolset::StampSkinPatch(const FSkinForgeStampRequest& Request)
{
	FSkinForgeStampResult Result;
	FString Error;
	if (!SkinForge::Stamp(Request, Result, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
	return Result;
}

TArray<FString> USkinForgeToolset::CaptureSkinPreview(const FSkinForgeCaptureRequest& Request)
{
	TArray<FString> Images;
	FString Error;
	if (!SkinForge::Capture(Request, Images, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
	return Images;
}

namespace
{
	template <typename T>
	T* LoadOrRaise(const FString& Path, const TCHAR* What, const TCHAR* Next)
	{
		T* Object = Path.IsEmpty() ? nullptr : LoadObject<T>(nullptr, *Path);
		if (!Object)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' is not a %s that loads. %s"), *Path, What, Next));
		}
		return Object;
	}
}

void USkinForgeToolset::OpenSkinForge(const FString& Set)
{
	USkinForgeSet* Loaded = nullptr;
	if (!Set.IsEmpty())
	{
		Loaded = LoadOrRaise<USkinForgeSet>(Set, TEXT("skin set"), TEXT("Create Skin Set makes one."));
		if (!Loaded)
		{
			return;
		}
	}
	SkinForge::OpenPanel(Loaded);
}

FString USkinForgeToolset::CreateSkinSet(const FString& Name, const TArray<FString>& Meshes)
{
	TArray<USkeletalMesh*> Loaded;
	for (const FString& Path : Meshes)
	{
		USkeletalMesh* Mesh = LoadOrRaise<USkeletalMesh>(Path, TEXT("skeletal mesh"), TEXT("Pass skeletal mesh asset paths."));
		if (!Mesh)
		{
			return FString();
		}
		Loaded.Add(Mesh);
	}
	FString Error;
	USkinForgeSet* Set = SkinForge::CreateSet(Name, SkinForge::SetFolder, Loaded, Error);
	if (!Set)
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
		return FString();
	}
	return Set->GetPathName();
}

TArray<FString> USkinForgeToolset::AddStarterSkinDecals()
{
	TArray<FString> Paths;
	FString Error;
	for (USkinForgeDecal* Decal : SkinForge::AddStarterDecals(SkinForge::DecalFolder, Error))
	{
		Paths.Add(Decal->GetPathName());
	}
	if (Paths.Num() == 0)
	{
		UKismetSystemLibrary::RaiseScriptError(Error.IsEmpty() ? TEXT("No starter decals were made.") : Error);
	}
	return Paths;
}

TArray<FString> USkinForgeToolset::ImportSkinMarks(const FString& Directory, const FString& ContentFolder)
{
	TArray<FString> Paths;
	if (!ContentFolder.StartsWith(TEXT("/Game/_Generated/")))
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("ContentFolder must be under /Game/_Generated/, e.g. /Game/_Generated/SkinForge/MyMarks."));
		return Paths;
	}
	FString Error;
	const FString Dir = FPaths::ConvertRelativePathToFull(Directory);
	for (USkinForgeDecal* Decal : SkinForge::ImportMarkLibrary(Dir, ContentFolder, Dir, Error))
	{
		Paths.Add(Decal->GetPathName());
	}
	if (Paths.Num() == 0)
	{
		UKismetSystemLibrary::RaiseScriptError(Error.IsEmpty() ? FString::Printf(TEXT("No marks were made from %s."), *Dir) : Error);
	}
	return Paths;
}

int32 USkinForgeToolset::StampSkinLayer(const FString& Set, const FString& Decal, FVector Start, FVector Direction)
{
	USkinForgeSet* LoadedSet = LoadOrRaise<USkinForgeSet>(Set, TEXT("skin set"), TEXT("Create Skin Set makes one."));
	USkinForgeDecal* LoadedDecal = LoadedSet ? LoadOrRaise<USkinForgeDecal>(Decal, TEXT("skin decal"), TEXT("Add Starter Skin Decals lists some.")) : nullptr;
	if (!LoadedSet || !LoadedDecal)
	{
		return INDEX_NONE;
	}
	FString Error;
	const int32 Index = SkinForge::AddLayerByRay(LoadedSet, LoadedDecal, Start, Direction, Error);
	if (Index == INDEX_NONE)
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
	return Index;
}

namespace
{
	/** An ink from JSON: [r, g, b] sRGB 0..1, "#RRGGBB", or a linear {"R":..,"G":..,"B":..}. */
	bool InkFromJson(const TSharedPtr<FJsonValue>& Value, FLinearColor& Out)
	{
		const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
		FString Text;
		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (Value->TryGetArray(Array) && Array->Num() >= 3)
		{
			Out = FLinearColor(FLinearColor::FromSRGBColor(FColor(
				uint8(FMath::Clamp(FMath::RoundToInt((*Array)[0]->AsNumber() * 255.0), 0, 255)),
				uint8(FMath::Clamp(FMath::RoundToInt((*Array)[1]->AsNumber() * 255.0), 0, 255)),
				uint8(FMath::Clamp(FMath::RoundToInt((*Array)[2]->AsNumber() * 255.0), 0, 255)))));
			return true;
		}
		if (Value->TryGetString(Text) && !Text.IsEmpty())
		{
			Out = FLinearColor::FromSRGBColor(FColor::FromHex(Text));
			return true;
		}
		if (Value->TryGetObject(Object))
		{
			double R = 0, G = 0, B = 0;
			(*Object)->TryGetNumberField(TEXT("R"), R);
			(*Object)->TryGetNumberField(TEXT("G"), G);
			(*Object)->TryGetNumberField(TEXT("B"), B);
			Out = FLinearColor(float(R), float(G), float(B), 1.f);
			return true;
		}
		return false;
	}
}

FString USkinForgeToolset::EditSkinLayer(const FString& Set, int32 Layer, const FString& Properties)
{
	USkinForgeSet* Loaded = LoadOrRaise<USkinForgeSet>(Set, TEXT("skin set"), TEXT("Create Skin Set makes one."));
	if (!Loaded)
	{
		return FString();
	}
	if (!Loaded->Layers.IsValidIndex(Layer))
	{
		UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("Layer %d is not one of the set's %d layers. Indexes start at 0, bottom layer first."), Layer, Loaded->Layers.Num()));
		return FString();
	}
	TSharedPtr<FJsonObject> Json;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Properties), Json) || !Json.IsValid())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("Properties is not a JSON object. Pass e.g. {\"ColourBlend\":\"Ink\"}."));
		return FString();
	}

	// Every name must be a property of the layer: a misspelt one is an error, not silently ignored.
	FSkinForgeLayer Edited = Loaded->Layers[Layer];
	const UScriptStruct* Struct = FSkinForgeLayer::StaticStruct();

	// The mark's own settings: a preset first, so the other keys can adjust what it set.
	FString PresetName;
	if (Json->TryGetStringField(TEXT("Preset"), PresetName))
	{
		FString Error;
		if (!SkinForge::ApplyPreset(Edited, PresetName, Error))
		{
			UKismetSystemLibrary::RaiseScriptError(Error);
			return FString();
		}
	}
	for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Json->Values)
	{
		if (Pair.Key.Equals(TEXT("Preset"), ESearchCase::IgnoreCase))
		{
			continue;
		}
		const USkinForgeDecal* Mark = Edited.Decal;
		if (Pair.Key.Equals(TEXT("Controls"), ESearchCase::IgnoreCase))
		{
			const TSharedPtr<FJsonObject>* Controls = nullptr;
			if (!Pair.Value->TryGetObject(Controls) || !Mark)
			{
				UKismetSystemLibrary::RaiseScriptError(TEXT("Controls takes an object of control name to value, on a layer with a mark, e.g. {\"Wear\":0.6}."));
				return FString();
			}
			for (const TPair<FString, TSharedPtr<FJsonValue>>& C : (*Controls)->Values)
			{
				const FSkinForgeControl* Control = Mark->FindControl(C.Key);
				if (!Control)
				{
					TArray<FString> Names;
					for (const FSkinForgeControl& Known : Mark->Controls) Names.Add(Known.Name);
					UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("%s has no control '%s'. It has: %s."), *Mark->GetName(), *C.Key,
						Names.Num() ? *FString::Join(Names, TEXT(", ")) : TEXT("none")));
					return FString();
				}
				double Number = 0.0;
				bool bBool = false;
				FString Option;
				float Value = 0.f;
				// A string first: a JSON string would read as the number 0.
				if (C.Value->Type == EJson::String && C.Value->TryGetString(Option) && Control->Type == ESkinForgeControlType::Choice)
				{
					const int32 Index = Control->Options.IndexOfByPredicate([&Option](const FString& O) { return O.Equals(Option, ESearchCase::IgnoreCase); });
					if (Index == INDEX_NONE)
					{
						UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' is not an option of %s: %s."), *Option, *Control->Name, *FString::Join(Control->Options, TEXT(", "))));
						return FString();
					}
					Value = float(Index);
				}
				else if (C.Value->Type == EJson::Boolean && C.Value->TryGetBool(bBool)) Value = bBool ? 1.f : 0.f;
				else if (C.Value->Type == EJson::Number && C.Value->TryGetNumber(Number)) Value = float(Number);
				else
				{
					UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("%s takes a number%s."), *Control->Name, Control->Type == ESkinForgeControlType::Choice ? TEXT(" or an option name") : TEXT("")));
					return FString();
				}
				Edited.ControlValues.Add(Control->Name, Value);
			}
			continue;
		}
		if (Pair.Key.Equals(TEXT("Slots"), ESearchCase::IgnoreCase) || Pair.Key.Equals(TEXT("Inks"), ESearchCase::IgnoreCase))
		{
			const TSharedPtr<FJsonObject>* Slots = nullptr;
			if (!Pair.Value->TryGetObject(Slots) || !Mark)
			{
				UKismetSystemLibrary::RaiseScriptError(TEXT("Slots takes an object of slot name to ink, on a layer with a mark, e.g. {\"Line\":[0.06,0.06,0.07]}."));
				return FString();
			}
			for (const TPair<FString, TSharedPtr<FJsonValue>>& S : (*Slots)->Values)
			{
				const FSkinForgeInkSlot* Slot = Mark->Slots.FindByPredicate([&S](const FSkinForgeInkSlot& K) { return K.Name.Equals(S.Key, ESearchCase::IgnoreCase); });
				const bool bImplicit = !Slot && Mark->Slots.Num() == 0 && S.Key.Equals(TEXT("Ink"), ESearchCase::IgnoreCase);
				if (!Slot && !bImplicit)
				{
					TArray<FString> Names;
					for (const FSkinForgeInkSlot& Known : Mark->Slots) Names.Add(Known.Name);
					UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("%s has no ink slot '%s'. It has: %s."), *Mark->GetName(), *S.Key,
						Names.Num() ? *FString::Join(Names, TEXT(", ")) : TEXT("Ink (the layer Tint)")));
					return FString();
				}
				if (S.Value->IsNull())
				{
					if (bImplicit) Edited.Tint = FLinearColor::White; else Edited.SlotColours.Remove(Slot->Name);
					continue;
				}
				FLinearColor Ink;
				if (!InkFromJson(S.Value, Ink))
				{
					UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("The ink for '%s' is not [r,g,b] (sRGB 0..1), \"#RRGGBB\" or {\"R\",\"G\",\"B\"} (linear)."), *S.Key));
					return FString();
				}
				if (bImplicit) Edited.Tint = Ink; else Edited.SlotColours.Add(Slot->Name, Ink);
			}
			continue;
		}

		FProperty* Found = nullptr;
		TArray<FString> Names;
		for (TFieldIterator<FProperty> It(Struct); It; ++It)
		{
			Names.Add(It->GetName());
			if (It->GetName().Equals(Pair.Key, ESearchCase::IgnoreCase))
			{
				Found = *It;
			}
		}
		if (!Found)
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("A layer has no property '%s'. It has: %s."), *Pair.Key, *FString::Join(Names, TEXT(", "))));
			return FString();
		}
		if (!FJsonObjectConverter::JsonValueToUProperty(Pair.Value, Found, Found->ContainerPtrToValuePtr<void>(&Edited), 0, 0))
		{
			UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' could not be set from the value given. Enums take their names (\"Ink\"), colours {\"R\":1,\"G\":1,\"B\":1,\"A\":1}, assets their paths."), *Pair.Key));
			return FString();
		}
	}

	Loaded->Modify();
	Loaded->Layers[Layer] = Edited;
	Loaded->MarkPackageDirty();
	SkinForge::RefreshPanel(Loaded);

	TSharedPtr<FJsonObject> Out = FJsonObjectConverter::UStructToJsonObject(Edited);
	if (Out.IsValid() && Edited.Decal)
	{
		TSharedRef<FJsonObject> Controls = MakeShared<FJsonObject>();
		for (const FSkinForgeControl& Control : Edited.Decal->Controls)
		{
			const float Value = Edited.Decal->GetControlValue(Edited, Control);
			if (Control.Type == ESkinForgeControlType::Choice && Control.Options.IsValidIndex(FMath::RoundToInt(Value)))
			{
				Controls->SetStringField(Control.Name, Control.Options[FMath::RoundToInt(Value)]);
			}
			else
			{
				Controls->SetNumberField(Control.Name, Value);
			}
		}
		Out->SetObjectField(TEXT("ResolvedControls"), Controls);
		TSharedRef<FJsonObject> Slots = MakeShared<FJsonObject>();
		for (const FSkinForgeInkSlot& Slot : Edited.Decal->Slots)
		{
			Slots->SetStringField(Slot.Name, TEXT("#") + Edited.Decal->GetSlotColour(Edited, Slot).ToFColorSRGB().ToHex().Left(6));
		}
		Out->SetObjectField(TEXT("ResolvedSlots"), Slots);
	}
	FString Text;
	if (Out.IsValid())
	{
		FJsonSerializer::Serialize(Out.ToSharedRef(), TJsonWriterFactory<>::Create(&Text));
	}
	return Text;
}

FSkinForgeStampResult USkinForgeToolset::BakeSkinSet(const FString& Set, const FString& Version)
{
	FSkinForgeStampResult Result;
	USkinForgeSet* Loaded = LoadOrRaise<USkinForgeSet>(Set, TEXT("skin set"), TEXT("Create Skin Set makes one."));
	FString Error;
	if (Loaded && !SkinForge::BakeSet(Loaded, Version, Result, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
	SkinForge::RefreshPanel(Loaded);
	return Result;
}

TArray<FSkinForgeVersionInfo> USkinForgeToolset::ListSkinVersions(const FString& Set)
{
	USkinForgeSet* Loaded = LoadOrRaise<USkinForgeSet>(Set, TEXT("skin set"), TEXT("Create Skin Set makes one."));
	return Loaded ? SkinForge::ListVersions(Loaded) : TArray<FSkinForgeVersionInfo>();
}

TArray<FString> USkinForgeToolset::FindSkinMaterialUsers(const FString& Set)
{
	USkinForgeSet* Loaded = LoadOrRaise<USkinForgeSet>(Set, TEXT("skin set"), TEXT("Create Skin Set makes one."));
	return Loaded ? SkinForge::FindMaterialUsers(Loaded) : TArray<FString>();
}

FSkinForgeApplyReport USkinForgeToolset::ApplySkinVersion(const FString& Set, const FString& Version, const TArray<FString>& Targets, bool bSave)
{
	FSkinForgeApplyReport Report;
	USkinForgeSet* Loaded = LoadOrRaise<USkinForgeSet>(Set, TEXT("skin set"), TEXT("Create Skin Set makes one."));
	if (!Loaded)
	{
		return Report;
	}

	// Named targets join the set's list, so the panel shows them and Revert reaches them later.
	TArray<UObject*> Objects;
	if (Targets.Num() > 0)
	{
		Loaded->Modify();
		for (const FString& Path : Targets)
		{
			UObject* Object = FSoftObjectPath(Path).TryLoad();
			if (!Object)
			{
				UKismetSystemLibrary::RaiseScriptError(FString::Printf(TEXT("'%s' does not load. Pass asset paths, or actor paths from the level."), *Path));
				return Report;
			}
			Loaded->ApplyTo.AddUnique(TSoftObjectPtr<UObject>(Object));
			Objects.Add(Object);
		}
		Loaded->MarkPackageDirty();
	}
	else
	{
		for (const TSoftObjectPtr<UObject>& Target : Loaded->ApplyTo)
		{
			Objects.Add(Target.LoadSynchronous());
		}
	}

	FString Error;
	if (!SkinForge::ApplyVersion(Loaded, Version, Objects, Report, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
		return Report;
	}
	if (bSave)
	{
		TArray<UPackage*> Packages = { Loaded->GetPackage() };
		for (const FString& Name : Report.Packages)
		{
			if (UPackage* Package = FindPackage(nullptr, *Name))
			{
				Packages.AddUnique(Package);
			}
		}
		UEditorLoadingAndSavingUtils::SavePackages(Packages, false);
		Report.Packages.Reset();
	}
	SkinForge::RefreshPanel(Loaded);
	return Report;
}

TArray<FSkinForgeTargetState> USkinForgeToolset::DescribeSkinApply(const FString& Set)
{
	TArray<FSkinForgeTargetState> States;
	USkinForgeSet* Loaded = LoadOrRaise<USkinForgeSet>(Set, TEXT("skin set"), TEXT("Create Skin Set makes one."));
	if (Loaded)
	{
		for (const TSoftObjectPtr<UObject>& Target : Loaded->ApplyTo)
		{
			FSkinForgeTargetState State = SkinForge::DescribeTarget(Loaded, Target.LoadSynchronous());
			if (State.Target.IsEmpty())
			{
				State.Target = Target.ToSoftObjectPath().ToString();
			}
			States.Add(State);
		}
	}
	return States;
}

FSkinForgeRenderReady USkinForgeToolset::IsSkinSetRenderReady(const FString& Set, const FString& Version)
{
	USkinForgeSet* Loaded = LoadOrRaise<USkinForgeSet>(Set, TEXT("skin set"), TEXT("Create Skin Set makes one."));
	return Loaded ? SkinForge::RenderReady(Loaded, Version) : FSkinForgeRenderReady();
}

void USkinForgeToolset::RestoreSkinVersionLayers(const FString& Set, const FString& Version)
{
	USkinForgeSet* Loaded = LoadOrRaise<USkinForgeSet>(Set, TEXT("skin set"), TEXT("Create Skin Set makes one."));
	FString Error;
	if (Loaded && !SkinForge::RestoreVersionLayers(Loaded, Version, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
	SkinForge::RefreshPanel(Loaded);
}

void USkinForgeToolset::AimSkinForgeCamera(FVector Target, FVector ViewFrom, float DistanceCm)
{
	FString Error;
	if (!SkinForge::AimPanelCamera(Target, ViewFrom, DistanceCm, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
}

void USkinForgeToolset::SetSkinForgeView(const FString& View)
{
	FString Error;
	if (!SkinForge::SetPanelView(View, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
}

FString USkinForgeToolset::ArmSkinDecal(const FString& Decal, const FString& StampMode)
{
	USkinForgeDecal* Loaded = nullptr;
	if (!Decal.IsEmpty())
	{
		Loaded = LoadOrRaise<USkinForgeDecal>(Decal, TEXT("skin decal"), TEXT("Add Starter Skin Decals lists some."));
		if (!Loaded)
		{
			return FString();
		}
	}
	int32 Keep = -1;
	if (StampMode.Equals(TEXT("once"), ESearchCase::IgnoreCase)) Keep = 0;
	else if (StampMode.Equals(TEXT("keep"), ESearchCase::IgnoreCase)) Keep = 1;
	else if (!StampMode.IsEmpty())
	{
		UKismetSystemLibrary::RaiseScriptError(TEXT("StampMode is \"once\", \"keep\" or empty."));
		return FString();
	}
	FString State, Error;
	if (!SkinForge::ArmPanel(Loaded, Keep, State, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
	return State;
}

FString USkinForgeToolset::DescribeSkinMark(const FString& Decal)
{
	USkinForgeDecal* Loaded = LoadOrRaise<USkinForgeDecal>(Decal, TEXT("skin decal"), TEXT("Add Starter Skin Decals lists some."));
	return Loaded ? SkinForge::DescribeMark(Loaded) : FString();
}

FString USkinForgeToolset::ClickSkinForgeViewport(int32 X, int32 Y, int32 ToX, int32 ToY, bool bShift, bool bAlt)
{
	FString What, Error;
	if (!SkinForge::ClickPanel(X, Y, ToX, ToY, bShift, bAlt, What, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
	return What;
}

FString USkinForgeToolset::CaptureSkinForgePanel()
{
	const FString File = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("SkinForge/Panel")
		/ (FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + TEXT(".png")));
	FString Error;
	if (!SkinForge::CapturePanel(File, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
		return FString();
	}
	return File;
}

TArray<FSkinForgeChannelUse> USkinForgeToolset::ProbeTextureChannels(const FString& Material, const FString& Parameter)
{
	TArray<FSkinForgeChannelUse> Uses;
	FString Error;
	if (!SkinForge::ProbeTextureParameter(Material, Parameter, Uses, Error))
	{
		UKismetSystemLibrary::RaiseScriptError(Error);
	}
	return Uses;
}
