// The Black Knight: Beginnings

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_BKMeleeAttack.generated.h"

class ABKEnemyCharacter;

/**
 *  Starts a randomized melee combo on the enemy if the target (task's
 *  Blackboard key) is within its attack range, and finishes when the combo
 *  ends. Aborting cancels the attack.
 */
UCLASS(DisplayName = "BK Melee Attack")
class BLACKKNIGHT_API UBTTask_BKMeleeAttack : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

protected:
	/** Allowance beyond the enemy's AttackRange so a target stepping back mid-approach still gets swung at. */
	UPROPERTY(EditAnywhere, Category = "Attack", Meta = (ClampMin = 0, Units = "cm"))
	float RangeTolerance{60.0f};

	TWeakObjectPtr<UBehaviorTreeComponent> OwnerCompWeak;
	TWeakObjectPtr<ABKEnemyCharacter> EnemyWeak;
	FDelegateHandle AttackEndedHandle;

public:
	UBTTask_BKMeleeAttack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;

protected:
	void OnAttackEnded(bool bInterrupted);

	void Unbind();
};
