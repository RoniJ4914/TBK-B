// The Black Knight: Beginnings

#include "BKCreateTestLevelCommandlet.h"

#if WITH_EDITOR
#include "ActorFactories/ActorFactory.h"
#include "BKTestDummyEnemy.h"
#include "Builders/CubeBuilder.h"
#include "EngineUtils.h"
#include "Editor/UnrealEdEngine.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "NavMesh/NavMeshBoundsVolume.h"
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

	// Milestone 2: navigation for enemy AI. The navmesh itself is generated at
	// runtime (RuntimeGeneration=Dynamic in DefaultEngine.ini), so only the bounds
	// are saved here.
	if (ANavMeshBoundsVolume* NavBounds = World->SpawnActor<ANavMeshBoundsVolume>(FVector{0.0, 0.0, 0.0}, FRotator::ZeroRotator))
	{
		UCubeBuilder* CubeBuilder = NewObject<UCubeBuilder>();
		CubeBuilder->X = 10000.0f;
		CubeBuilder->Y = 10000.0f;
		CubeBuilder->Z = 1000.0f;
		UActorFactory::CreateBrushForVolumeActor(NavBounds, CubeBuilder);
	}

	// Milestone 2 acceptance target: a test dummy enemy in front of the player start.
	const APlayerStart* PlayerStart = nullptr;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		PlayerStart = *It;
		break;
	}

	const FTransform StartTransform = IsValid(PlayerStart) ? PlayerStart->GetActorTransform() : FTransform::Identity;
	const FVector DummyLocation = StartTransform.GetLocation() + StartTransform.GetRotation().GetForwardVector() * 900.0;
	const FRotator DummyRotation = (StartTransform.GetLocation() - DummyLocation).Rotation();

	if (!IsValid(World->SpawnActor<ABKTestDummyEnemy>(DummyLocation, FRotator{0.0, DummyRotation.Yaw, 0.0})))
	{
		UE_LOG(LogTemp, Error, TEXT("BKCreateTestLevel: failed to spawn the test dummy enemy."));
		return 1;
	}

	static const FString PackagePath{TEXT("/Game/Levels/L_TestLevel")};

	if (!UEditorLoadingAndSavingUtils::SaveMap(World, PackagePath))
	{
		UE_LOG(LogTemp, Error, TEXT("BKCreateTestLevel: SaveMap to %s failed."), *PackagePath);
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("BKCreateTestLevel: saved %s (test dummy at %s)"), *PackagePath, *DummyLocation.ToString());
	return 0;
#else
	UE_LOG(LogTemp, Error, TEXT("BKCreateTestLevel: editor-only commandlet, WITH_EDITOR=0."));
	return 1;
#endif
}
