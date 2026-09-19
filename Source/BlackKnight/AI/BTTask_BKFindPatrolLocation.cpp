// The Black Knight: Beginnings

#include "BTTask_BKFindPatrolLocation.h"

#include "AIController.h"
#include "BKEnemyCharacter.h"
#include "NavigationSystem.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"

UBTTask_BKFindPatrolLocation::UBTTask_BKFindPatrolLocation()
{
	NodeName = TEXT("BK Find Patrol Location");

	BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, BlackboardKey));
	HomeKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, HomeKey));
}

void UBTTask_BKFindPatrolLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (const auto* BlackboardAsset{GetBlackboardAsset()})
	{
		HomeKey.ResolveSelectedKey(*BlackboardAsset);
	}
}

EBTNodeResult::Type UBTTask_BKFindPatrolLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const auto* Controller{OwnerComp.GetAIOwner()};
	const auto* Enemy{IsValid(Controller) ? Cast<ABKEnemyCharacter>(Controller->GetPawn()) : nullptr};
	auto* Blackboard{OwnerComp.GetBlackboardComponent()};
	auto* NavigationSystem{FNavigationSystem::GetCurrent<UNavigationSystemV1>(OwnerComp.GetWorld())};
	if (!IsValid(Enemy) || !IsValid(Blackboard) || !IsValid(NavigationSystem))
	{
		return EBTNodeResult::Failed;
	}

	const FVector Home{Blackboard->GetValue<UBlackboardKeyType_Vector>(HomeKey.GetSelectedKeyID())};

	FNavLocation PatrolLocation;
	if (!NavigationSystem->GetRandomReachablePointInRadius(Home, Enemy->GetPatrolRadius(), PatrolLocation))
	{
		return EBTNodeResult::Failed;
	}

	Blackboard->SetValue<UBlackboardKeyType_Vector>(BlackboardKey.GetSelectedKeyID(), PatrolLocation.Location);
	return EBTNodeResult::Succeeded;
}

FString UBTTask_BKFindPatrolLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: random point around %s"), *BlackboardKey.SelectedKeyName.ToString(), *HomeKey.SelectedKeyName.ToString());
}
