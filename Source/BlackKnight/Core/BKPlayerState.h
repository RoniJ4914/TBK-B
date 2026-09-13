// The Black Knight: Beginnings

#pragma once

#include "GameFramework/PlayerState.h"
#include "BKPlayerState.generated.h"

/**
 *  Per-player state that must survive pawn destruction and level travel.
 *
 *  Milestone 1 defines the type only. Milestone 3 adds currency and the
 *  equipped weapon here, because both must persist across the level travel
 *  built in Milestone 4.
 */
UCLASS()
class BLACKKNIGHT_API ABKPlayerState : public APlayerState
{
	GENERATED_BODY()
};
