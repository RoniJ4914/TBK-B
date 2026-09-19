// The Black Knight: Beginnings

#pragma once

#include "NativeGameplayTags.h"

/**
 *  Extends ALS's LocomotionAction gameplay tag slot (AAlsCharacter::SetLocomotionAction)
 *  with Black Knight combat states. ALS gates several of its own systems (Jump,
 *  mantling, sprinting) on "is LocomotionAction set to anything", so driving
 *  combat state through the same tag automatically suppresses those while the
 *  player is attacking or blocking, without touching ALS source.
 */
namespace BKLocomotionActionTags
{
	BLACKKNIGHT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attacking)
	BLACKKNIGHT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Blocking)
	BLACKKNIGHT_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Staggered)
}
