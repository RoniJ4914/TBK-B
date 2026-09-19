// The Black Knight: Beginnings

#pragma once

#include "GameFramework/HUD.h"
#include "BKHUD.generated.h"

class UBKHUDWidget;

/**
 *  Creates the player HUD widget and draws the lock-on marker over the
 *  camera's current lock-on target.
 */
UCLASS()
class BLACKKNIGHT_API ABKHUD : public AHUD
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	TObjectPtr<UBKHUDWidget> HUDWidget;

	virtual void BeginPlay() override;

public:
	virtual void DrawHUD() override;
};
