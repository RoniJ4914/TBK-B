// The Black Knight: Beginnings

#pragma once

#include "BKEnemyCharacter.h"
#include "BKTestDummyEnemy.generated.h"

/**
 *  Minimal concrete enemy for L_TestLevel: proves the damage/health/death
 *  pipeline end-to-end. Fragile, hits softly, and chains short combos.
 */
UCLASS()
class BLACKKNIGHT_API ABKTestDummyEnemy : public ABKEnemyCharacter
{
	GENERATED_BODY()

public:
	explicit ABKTestDummyEnemy(const FObjectInitializer& ObjectInitializer);
};
