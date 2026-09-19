// The Black Knight: Beginnings

#include "BTTask_BKMeleeAttack.h"

#include "AIController.h"
#include "BKEnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

UBTTask_BKMeleeAttack::UBTTask_BKMeleeAttack()
{
	NodeName = TEXT("BK Melee Attack");

	// Holds a delegate binding for the duration of an attack, so each AI needs its own instance.
	bCreateNodeInstance = true;

	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, BlackboardKey), AActor::StaticClass());
}

EBTNodeResult::Type UBTTask_BKMeleeAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const auto* Controller{OwnerComp.GetAIOwner()};
	auto* Enemy{IsValid(Controller) ? Cast<ABKEnemyCharacter>(Controller->GetPawn()) : nullptr};
	const auto* Blackboard{OwnerComp.GetBlackboardComponent()};
	const auto* Target{IsValid(Blackboard) ? Cast<AActor>(Blackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID())) : nullptr};

	if (!IsValid(Enemy) || !IsValid(Target) ||
	    FVector::Dist2D(Enemy->GetActorLocation(), Target->GetActorLocation()) > Enemy->GetAttackRange() + RangeTolerance)
	{
		return EBTNodeResult::Failed;
	}

	if (!Enemy->StartAIAttack())
	{
		return EBTNodeResult::Failed;
	}

	OwnerCompWeak = &OwnerComp;
	EnemyWeak = Enemy;
	AttackEndedHandle = Enemy->GetMelee()->OnAttackEnded.AddUObject(this, &ThisClass::OnAttackEnded);

	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_BKMeleeAttack::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const auto* Enemy{EnemyWeak.Get()};
	Unbind();

	if (IsValid(Enemy))
	{
		Enemy->GetMelee()->CancelAttack();
	}

	return EBTNodeResult::Aborted;
}

void UBTTask_BKMeleeAttack::OnAttackEnded(bool)
{
	auto* OwnerComp{OwnerCompWeak.Get()};
	Unbind();

	if (IsValid(OwnerComp))
	{
		FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
	}
}

void UBTTask_BKMeleeAttack::Unbind()
{
	if (const auto* Enemy{EnemyWeak.Get()}; IsValid(Enemy) && AttackEndedHandle.IsValid())
	{
		Enemy->GetMelee()->OnAttackEnded.Remove(AttackEndedHandle);
	}

	AttackEndedHandle.Reset();
	EnemyWeak.Reset();
	OwnerCompWeak.Reset();
}

FString UBTTask_BKMeleeAttack::GetStaticDescription() const
{
	return FString::Printf(TEXT("Combo at %s when in range"), *BlackboardKey.SelectedKeyName.ToString());
}
