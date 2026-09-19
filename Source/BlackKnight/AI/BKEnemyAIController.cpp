// The Black Knight: Beginnings

#include "BKEnemyAIController.h"

#include "BKDamageable.h"
#include "BKEnemyCharacter.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"
#include "Utility/AlsGameplayTags.h"

const FName ABKEnemyAIController::TargetActorKey{TEXT("TargetActor")};
const FName ABKEnemyAIController::HomeLocationKey{TEXT("HomeLocation")};
const FName ABKEnemyAIController::PatrolLocationKey{TEXT("PatrolLocation")};

ABKEnemyAIController::ABKEnemyAIController()
{
	// Same as ALS's own AI controller: keeps the controller (and its view point) on the pawn.
	bAttachToPawn = true;

	PerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 2000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 70.0f;
	SightConfig->SetMaxAge(LoseTargetDelay);
	// No team setup yet, so the player counts as neutral.
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	PerceptionComponent->ConfigureSense(*SightConfig);
	PerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void ABKEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	const auto* Enemy{Cast<ABKEnemyCharacter>(InPawn)};
	if (!IsValid(Enemy) || !IsValid(Enemy->GetBehaviorTree()))
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: possessed pawn has no Behavior Tree to run."), *GetName());
		return;
	}

	RunBehaviorTree(Enemy->GetBehaviorTree());

	if (IsValid(Blackboard))
	{
		Blackboard->SetValueAsVector(HomeLocationKey, InPawn->GetActorLocation());
	}

	PerceptionComponent->OnTargetPerceptionUpdated.AddUniqueDynamic(this, &ThisClass::OnTargetPerceptionUpdated);
}

void ABKEnemyAIController::OnUnPossess()
{
	PerceptionComponent->OnTargetPerceptionUpdated.RemoveDynamic(this, &ThisClass::OnTargetPerceptionUpdated);
	GetWorldTimerManager().ClearTimer(LoseTargetTimerHandle);

	Super::OnUnPossess();
}

void ABKEnemyAIController::Tick(const float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (const auto* Target{Cast<IBKDamageable>(GetTarget())}; Target != nullptr && !Target->IsAlive())
	{
		ClearTarget();
	}
}

AActor* ABKEnemyAIController::GetTarget() const
{
	return IsValid(Blackboard) ? Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey)) : nullptr;
}

void ABKEnemyAIController::OnTargetPerceptionUpdated(AActor* Actor, const FAIStimulus Stimulus)
{
	// Only players are targets for now; enemies don't fight each other.
	const auto* SensedPawn{Cast<APawn>(Actor)};
	const auto* Damageable{Cast<IBKDamageable>(Actor)};
	if (!IsValid(SensedPawn) || !SensedPawn->IsPlayerControlled() || Damageable == nullptr || !Damageable->IsAlive())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		GetWorldTimerManager().ClearTimer(LoseTargetTimerHandle);
		SetTarget(Actor);
	}
	else if (Actor == GetTarget())
	{
		GetWorldTimerManager().SetTimer(LoseTargetTimerHandle, this, &ThisClass::ClearTarget, LoseTargetDelay, false);
	}
}

void ABKEnemyAIController::SetTarget(AActor* NewTarget)
{
	if (!IsValid(Blackboard) || GetTarget() == NewTarget)
	{
		return;
	}

	Blackboard->SetValueAsObject(TargetActorKey, NewTarget);
	SetFocus(NewTarget);

	// Face the focus (via control rotation) while fighting instead of facing the direction of travel.
	if (auto* Enemy{Cast<ABKEnemyCharacter>(GetPawn())}; IsValid(Enemy))
	{
		Enemy->SetDesiredRotationMode(AlsRotationModeTags::ViewDirection);
	}
}

void ABKEnemyAIController::ClearTarget()
{
	GetWorldTimerManager().ClearTimer(LoseTargetTimerHandle);

	if (IsValid(Blackboard))
	{
		Blackboard->ClearValue(TargetActorKey);
	}
	ClearFocus(EAIFocusPriority::Gameplay);

	if (auto* Enemy{Cast<ABKEnemyCharacter>(GetPawn())}; IsValid(Enemy))
	{
		Enemy->SetDesiredRotationMode(AlsRotationModeTags::VelocityDirection);
	}
}
