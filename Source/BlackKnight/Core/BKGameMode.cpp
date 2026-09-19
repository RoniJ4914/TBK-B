// The Black Knight: Beginnings

#include "BKGameMode.h"

#include "BKGameState.h"
#include "BKHUD.h"
#include "BKPlayerController.h"
#include "BKPlayerState.h"
#include "Character/BKPlayerCharacter.h"

ABKGameMode::ABKGameMode()
{
	DefaultPawnClass = ABKPlayerCharacter::StaticClass();
	PlayerControllerClass = ABKPlayerController::StaticClass();
	GameStateClass = ABKGameState::StaticClass();
	PlayerStateClass = ABKPlayerState::StaticClass();
	HUDClass = ABKHUD::StaticClass();
}
