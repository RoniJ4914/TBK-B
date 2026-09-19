// The Black Knight: Beginnings

#pragma once

#include "GameFramework/CheatManager.h"
#include "BKCheatManager.generated.h"

/**
 *  Development-only console commands (cheat managers don't exist in Shipping).
 *
 *  BKCombatSelfTest: drives the possessed player through the real combat code
 *  path - lock-on, light/heavy combos, anim-notify hit traces, damage, death -
 *  against the nearest enemy, and logs PASSED/FAILED. Lets the Milestone 2
 *  acceptance test (kill the test dummy) run headlessly:
 *    UnrealEditor-Cmd.exe <uproject> -game -NullRHI -ExecCmds="EnableCheats,BKCombatSelfTest"
 *  Mode: "Mixed" (default; every third combo heavy), "Light" or "Heavy".
 */
UCLASS()
class BLACKKNIGHT_API UBKCheatManager : public UCheatManager
{
	GENERATED_BODY()

protected:
	FTimerHandle SelfTestTimerHandle;
	double SelfTestStartTime{0.0};
	int32 SelfTestCombosStarted{0};
	FString SelfTestMode;
	TWeakObjectPtr<AActor> SelfTestTarget;

	FTimerHandle ScreenshotTimerHandle;

public:
	UFUNCTION(Exec)
	void BKCombatSelfTest(const FString& Mode = TEXT("Mixed"));

	/**
	 * Visual check without a human at the editor: freezes enemy AI, swings the camera to face the
	 * player (YawOffset degrees off dead-front), stages Mode ("Block", "Attack" = light swing at its
	 * impact frame, "Hurt" = both sides damaged so HUD and enemy bars show), then saves
	 * Saved/Screenshots/BK_<Mode>.png. Needs a rendering run (not -NullRHI), e.g. -RenderOffScreen.
	 */
	UFUNCTION(Exec)
	void BKDebugScreenshot(const FString& Mode = TEXT("Block"), float YawOffset = 35.0f);

protected:
	void TickCombatSelfTest();

	void FinishCombatSelfTest(bool bPassed, const FString& Reason);
};
