// The Black Knight: Beginnings

#pragma once

#include "BKCombatCharacter.h"
#include "BKEnemyCharacter.generated.h"

class UBehaviorTree;
class UWidgetComponent;

/**
 *  Base class for AI enemies. Driven by ABKEnemyAIController running a
 *  Behavior Tree (BT_BK_Enemy by default): patrol around the spawn point,
 *  chase a perceived player, attack in range.
 *
 *  Future enemy types subclass this and tune the settings below (or swap the
 *  Behavior Tree) rather than re-implementing combat.
 */
UCLASS(AutoExpandCategories = ("Settings|Black Knight"))
class BLACKKNIGHT_API ABKEnemyCharacter : public ABKCombatCharacter
{
	GENERATED_BODY()

protected:
	/** Screen-space health bar over the head (UBKEnemyHealthBarWidget). */
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Black Knight")
	TObjectPtr<UWidgetComponent> HealthBarWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|AI")
	TObjectPtr<UBehaviorTree> BehaviorTree;

	/** Patrol points are picked within this radius of where the enemy spawned. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|AI", Meta = (ClampMin = 0, Units = "cm"))
	float PatrolRadius{800.0f};

	/** How close the enemy gets before swinging. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|AI", Meta = (ClampMin = 0, Units = "cm"))
	float AttackRange{130.0f};

	/** Each attack chains a random number of combo steps in [1, MaxComboSteps]. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|AI", Meta = (ClampMin = 1))
	int32 MaxComboSteps{3};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|AI", Meta = (ClampMin = 0, ClampMax = 1))
	float HeavyAttackChance{0.25f};

	/** Seconds the ragdolled body stays in the level after death. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|AI", Meta = (ClampMin = 0, Units = "s"))
	float CorpseLifeSpan{10.0f};

public:
	explicit ABKEnemyCharacter(const FObjectInitializer& ObjectInitializer);

	UBehaviorTree* GetBehaviorTree() const { return BehaviorTree; }

	float GetPatrolRadius() const { return PatrolRadius; }

	float GetAttackRange() const { return AttackRange; }

	/** Starts a randomized combo. Returns false if the enemy can't attack right now (mid-action, staggered, dead). */
	bool StartAIAttack();

protected:
	virtual void BeginPlay() override;

	virtual void HandleDeath() override;
};
