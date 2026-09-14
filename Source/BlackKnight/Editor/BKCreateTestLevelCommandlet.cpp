// The Black Knight: Beginnings

#include "BKCreateTestLevelCommandlet.h"

#if WITH_EDITOR
#include "Editor/UnrealEdEngine.h"
#include "FileHelpers.h"
#include "GameFramework/WorldSettings.h"
#include "UnrealEdGlobals.h"
#endif

int32 UBKCreateTestLevelCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	// Instantiated from the engine's own "Default" new-level template (the same
	// one File > New Level > Default uses) rather than NewBlankMap(), which
	// produces a truly empty world: no floor, no light, no sky. That left the
	// player falling through a lit-by-nothing void - a black screen.
	// Template_Default already ships a floor, DirectionalLight, SkyLight,
	// SkyAtmosphere, ExponentialHeightFog, VolumetricCloud and a PlayerStart.
	static const FString TemplatePath{TEXT("/Engine/Maps/Templates/Template_Default")};

	UWorld* World = UEditorLoadingAndSavingUtils::NewMapFromTemplate(TemplatePath, /*bSaveExistingMap=*/false);
	if (!IsValid(World))
	{
		UE_LOG(LogTemp, Error, TEXT("BKCreateTestLevel: NewMapFromTemplate(%s) failed."), *TemplatePath);
		return 1;
	}

	// No per-level override: this level inherits GlobalDefaultGameMode
	// (/Script/BlackKnight.BKGameMode) from DefaultEngine.ini.
	if (AWorldSettings* WorldSettings = World->GetWorldSettings())
	{
		WorldSettings->DefaultGameMode = nullptr;
	}

	static const FString PackagePath{TEXT("/Game/Levels/L_TestLevel")};

	if (!UEditorLoadingAndSavingUtils::SaveMap(World, PackagePath))
	{
		UE_LOG(LogTemp, Error, TEXT("BKCreateTestLevel: SaveMap to %s failed."), *PackagePath);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("BKCreateTestLevel: saved %s"), *PackagePath);
	return 0;
#else
	UE_LOG(LogTemp, Error, TEXT("BKCreateTestLevel: editor-only commandlet, WITH_EDITOR=0."));
	return 1;
#endif
}
