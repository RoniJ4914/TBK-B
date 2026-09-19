// The Black Knight: Beginnings

#include "BKCreateCombatInputAssetsCommandlet.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
	struct FBKCombatInputSpec
	{
		const TCHAR* AssetName;
		FKey Key;
	};

	const TCHAR* const InputFolder{TEXT("/Game/Input")};
	const TCHAR* const MappingContextName{TEXT("IMC_BK_Combat")};
	const TCHAR* const AlsMappingContextPath{TEXT("/ALS/ALS/Data/Input/IMC_Als_Default.IMC_Als_Default")};

	bool SaveAsset(UPackage* Package, UObject* Asset)
	{
		Package->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Asset);

		const FString PackageFileName{FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension())};

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

		return UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
	}

	/** Loads the asset if it's already on disk, otherwise creates it in a new package. */
	template <typename T>
	T* LoadOrCreate(const FString& AssetName, bool& bOutCreated)
	{
		const FString PackagePath{FString::Printf(TEXT("%s/%s"), InputFolder, *AssetName)};
		const FString ObjectPath{FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName)};

		bOutCreated = false;
		if (FPackageName::DoesPackageExist(PackagePath))
		{
			return LoadObject<T>(nullptr, *ObjectPath);
		}

		bOutCreated = true;
		return NewObject<T>(CreatePackage(*PackagePath), *AssetName, RF_Public | RF_Standalone);
	}
}

int32 UBKCreateCombatInputAssetsCommandlet::Main(const FString& Params)
{
	// Placeholder keyboard/mouse bindings; gamepad bindings and player remapping come later.
	const TArray<FBKCombatInputSpec> Specs{
		{TEXT("IA_BK_Block"), EKeys::RightMouseButton},
		{TEXT("IA_BK_LightAttack"), EKeys::LeftMouseButton},
		{TEXT("IA_BK_HeavyAttack"), EKeys::F},
		{TEXT("IA_BK_LockOn"), EKeys::MiddleMouseButton},
	};

	// Surface overlaps with ALS's own context instead of assuming keys are free. Ours is
	// added at a higher priority and consumes input, so an overlap silently disables the
	// ALS action on that key - acceptable only for ALS actions ABKPlayerCharacter doesn't bind.
	if (const auto* AlsContext{LoadObject<UInputMappingContext>(nullptr, AlsMappingContextPath)}; IsValid(AlsContext))
	{
		for (const FEnhancedActionKeyMapping& AlsMapping : AlsContext->GetMappings())
		{
			for (const FBKCombatInputSpec& Spec : Specs)
			{
				if (AlsMapping.Key == Spec.Key)
				{
					UE_LOG(LogTemp, Warning, TEXT("BKCreateCombatInputAssets: %s shares key %s with ALS action %s."),
					       Spec.AssetName, *Spec.Key.ToString(), *GetNameSafe(AlsMapping.Action));
				}
			}
		}
	}

	bool bContextCreated{false};
	auto* MappingContext{LoadOrCreate<UInputMappingContext>(MappingContextName, bContextCreated)};
	if (!IsValid(MappingContext))
	{
		UE_LOG(LogTemp, Error, TEXT("BKCreateCombatInputAssets: failed to load or create %s."), MappingContextName);
		return 1;
	}

	bool bContextChanged{bContextCreated};

	for (const FBKCombatInputSpec& Spec : Specs)
	{
		bool bActionCreated{false};
		auto* Action{LoadOrCreate<UInputAction>(Spec.AssetName, bActionCreated)};
		if (!IsValid(Action))
		{
			UE_LOG(LogTemp, Error, TEXT("BKCreateCombatInputAssets: failed to load or create %s."), Spec.AssetName);
			return 1;
		}

		// Boolean is UInputAction's default ValueType - correct for all of these digital actions.
		if (bActionCreated)
		{
			if (!SaveAsset(Action->GetPackage(), Action))
			{
				UE_LOG(LogTemp, Error, TEXT("BKCreateCombatInputAssets: failed to save %s."), Spec.AssetName);
				return 1;
			}
			UE_LOG(LogTemp, Display, TEXT("BKCreateCombatInputAssets: created %s"), Spec.AssetName);
		}

		const bool bAlreadyMapped{MappingContext->GetMappings().ContainsByPredicate([&](const FEnhancedActionKeyMapping& Mapping)
		{
			return Mapping.Action == Action && Mapping.Key == Spec.Key;
		})};

		if (!bAlreadyMapped)
		{
			MappingContext->MapKey(Action, Spec.Key);
			bContextChanged = true;
			UE_LOG(LogTemp, Display, TEXT("BKCreateCombatInputAssets: mapped %s -> %s"), Spec.AssetName, *Spec.Key.ToString());
		}
	}

	if (bContextChanged && !SaveAsset(MappingContext->GetPackage(), MappingContext))
	{
		UE_LOG(LogTemp, Error, TEXT("BKCreateCombatInputAssets: failed to save %s."), MappingContextName);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("BKCreateCombatInputAssets: done (%s %s)."), MappingContextName,
	       bContextChanged ? TEXT("updated") : TEXT("unchanged"));
	return 0;
}
#else
int32 UBKCreateCombatInputAssetsCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Error, TEXT("BKCreateCombatInputAssets: editor-only commandlet, WITH_EDITOR=0."));
	return 1;
}
#endif
