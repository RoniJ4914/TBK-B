// The Black Knight: Beginnings

#pragma once

#include "GameFramework/GameStateBase.h"
#include "BKGameState.generated.h"

/**
 *  World-wide, all-players state.
 *
 *  Milestone 1 defines the type only. Later milestones hang world-level state
 *  here: quest flags (Milestone 5) and boss encounter phase (Milestone 6).
 */
UCLASS()
class BLACKKNIGHT_API ABKGameState : public AGameStateBase
{
	GENERATED_BODY()
};
