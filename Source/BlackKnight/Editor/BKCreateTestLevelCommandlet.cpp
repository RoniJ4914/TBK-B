// The Black Knight: Beginnings

#include "BKCreateTestLevelCommandlet.h"

#if WITH_EDITOR
#include "Editor/UnrealEdEngine.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "UnrealEdGlobals.h"
#endif

int32 UBKCreateTestLevelCommandlet::Main(const FString& Params)
{
#if WITH_EDITOR
	UWorld* World = UEditorLoadingAndSavingUtils::NewBlankMap(/*bSaveExistingMap=*/false);
	if (!IsValid(World))
	{
		UE_LOG(LogTemp, Error, TEXT("BKCreateTestLevel: NewBlankMap failed."));
		return 1;
	}

	World->SpawnActor<APlayerStart>(FVector{0.0f, 0.0f, 100.0f}, FRotator::ZeroRotator);

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
