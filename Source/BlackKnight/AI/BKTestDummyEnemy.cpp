// The Black Knight: Beginnings

#include "BKTestDummyEnemy.h"

#include "BKHealthComponent.h"

ABKTestDummyEnemy::ABKTestDummyEnemy(const FObjectInitializer& ObjectInitializer) : Super{ObjectInitializer}
{
	Health->SetMaxHealth(60.0f);
	Melee->SetDamageMultiplier(0.5f);

	MaxComboSteps = 2;
	HeavyAttackChance = 0.15f;
}
