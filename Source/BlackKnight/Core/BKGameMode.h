// The Black Knight: Beginnings

#pragma once

#include "GameFramework/GameModeBase.h"
#include "BKGameMode.generated.h"

/**
 *  Default game mode for The Black Knight.
 *
 *  Single-player, so AGameModeBase is sufficient; there is no match state or
 *  round lifecycle to model. Every class default is set in C++ so the mode is
 *  usable directly, without a Blueprint wrapper.
 */
UCLASS()
class BLACKKNIGHT_API ABKGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ABKGameMode();
};
