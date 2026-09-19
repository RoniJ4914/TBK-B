// The Black Knight: Beginnings

#pragma once

#include "GameFramework/PlayerController.h"
#include "BKPlayerController.generated.h"

/**
 *  Player controller for The Black Knight.
 *
 *  Input bindings live on the pawn (see ABKPlayerCharacter) so that they follow
 *  possession. This class is the home for controller-scoped concerns instead:
 *  development cheats (UBKCheatManager) now, and the quest log and vendor UI
 *  in Milestones 3 and 5.
 */
UCLASS()
class BLACKKNIGHT_API ABKPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ABKPlayerController();
};
