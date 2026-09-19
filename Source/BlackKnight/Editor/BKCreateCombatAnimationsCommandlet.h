// The Black Knight: Beginnings

#pragma once

#include "Commandlets/Commandlet.h"
#include "BKCreateCombatAnimationsCommandlet.generated.h"

/**
 *  Editor-only commandlet that builds the attack combo montages under
 *  /Game/Combat/Animations from the original template's unarmed attack clips.
 *
 *  1. Retarget: each source clip (UE5 Manny skeleton) is baked onto ALS's own
 *     skeleton (SK_Als). Rotations are matched in component space, so the
 *     5-bone Manny spine maps onto ALS's 3-bone spine without losing upper-body
 *     twist; translations come from ALS's reference pose, so bone lengths stay
 *     ALS's (playing the Manny clips directly let Manny's bone offsets through,
 *     visibly detaching the hands). Pelvis/root motion is scaled by hip height.
 *     IK bones are snapped to their hands/feet.
 *  2. Montages: one montage per combo step, in ALS's full-body "PostLocomotion"
 *     slot, with cubic blend in/out so combo steps and the return to idle
 *     cross-fade. Hit and combo-window notify times come from the template's
 *     montages; ALS layering curves (Layer*, ViewBlock) come from ALS's roll.
 *
 *  Only four unarmed source clips exist, so combo steps reuse clips at
 *  different play rates until dedicated combat animations are added.
 *
 *  3. Block: no guard clip exists, so a held high-guard pose is built from
 *     ALS's standing pose (two-bone IK on each arm, curled fingers) and looped
 *     by AM_BK_Block. It is full-body, so blocking plants the character; swap
 *     the pose for a real guard clip (ideally with locomotion) later.
 *
 *  ALS layering curves (Layer*, ViewBlock) are written onto the baked
 *  sequences, not the montages: montage-level curves are never evaluated.
 *
 *  Run with: UnrealEditor-Cmd.exe <uproject> -run=BKCreateCombatAnimations
 *  Always rebuilds (all outputs are generated).
 */
UCLASS()
class UBKCreateCombatAnimationsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
