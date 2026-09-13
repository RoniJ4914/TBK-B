// The Black Knight: Beginnings

#pragma once

#include "Commandlets/Commandlet.h"
#include "BKCreateTestLevelCommandlet.generated.h"

/**
 *  Editor-only, one-shot commandlet that creates the Milestone 1 acceptance
 *  level: an empty map with a PlayerStart, no per-level game mode override
 *  (so it inherits the project's ABKGameMode), saved to /Game/Levels.
 *
 *  Run with: UnrealEditor-Cmd.exe <uproject> -run=BKCreateTestLevel
 *
 *  This exists so the level can be produced deterministically and re-run if
 *  ever needed, without hand-editing binary .umap files.
 */
UCLASS()
class UBKCreateTestLevelCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
