// The Black Knight: Beginnings

#pragma once

#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_BKFindPatrolLocation.generated.h"

/**
 *  Picks a random navigable point within the enemy's PatrolRadius of
 *  HomeKey and writes it to the task's Blackboard key.
 */
UCLASS(DisplayName = "BK Find Patrol Location")
class BLACKKNIGHT_API UBTTask_BKFindPatrolLocation : public UBTTask_BlackboardBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector HomeKey;

	UBTTask_BKFindPatrolLocation();

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;
};
