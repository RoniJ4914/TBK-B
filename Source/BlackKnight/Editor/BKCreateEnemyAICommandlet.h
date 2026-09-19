// The Black Knight: Beginnings

#pragma once

#include "Commandlets/Commandlet.h"
#include "BKCreateEnemyAICommandlet.generated.h"

/**
 *  Editor-only commandlet that generates the enemy Blackboard and Behavior Tree
 *  under /Game/AI:
 *
 *    BB_BK_Enemy: TargetActor (Actor), HomeLocation (Vector), PatrolLocation (Vector)
 *
 *    BT_BK_Enemy:
 *      Selector
 *      |- Combat  [Blackboard: TargetActor is set, aborts both]
 *      |    MoveTo TargetActor -> BK Melee Attack -> Wait
 *      |- Patrol
 *           BK Find Patrol Location -> MoveTo PatrolLocation -> Wait
 *
 *  The tree is built from runtime nodes only; the Behavior Tree editor
 *  generates its graph from them the first time the asset is opened, after
 *  which it can be edited normally.
 *
 *  Run with: UnrealEditor-Cmd.exe <uproject> -run=BKCreateEnemyAI
 *  Always rebuilds both assets.
 */
UCLASS()
class UBKCreateEnemyAICommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
