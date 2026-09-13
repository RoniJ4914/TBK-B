// The Black Knight: Beginnings

#pragma once

#include "AlsCameraComponent.h"
#include "BKCameraComponent.generated.h"

/**
 *  Project camera for the player character.
 *
 *  Exists as a subclass of UAlsCameraComponent for two reasons:
 *   1. UAlsCameraComponent::Settings is protected, so only a subclass can supply
 *      its default settings asset from C++ instead of requiring a Blueprint.
 *   2. It is the seam where Milestone 2's lock-on camera will live.
 */
UCLASS(ClassGroup = "BlackKnight", Meta = (BlueprintSpawnableComponent))
class BLACKKNIGHT_API UBKCameraComponent : public UAlsCameraComponent
{
	GENERATED_BODY()

public:
	UBKCameraComponent();
};
