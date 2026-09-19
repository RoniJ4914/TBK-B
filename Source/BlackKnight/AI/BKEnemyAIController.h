// The Black Knight: Beginnings

#pragma once

#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BKEnemyAIController.generated.h"

class UAISenseConfig_Sight;

/**
 *  Runs the possessed ABKEnemyCharacter's Behavior Tree and feeds its
 *  Blackboard from sight perception: TargetActor is set while a living player
 *  is seen (and kept for LoseTargetDelay after losing sight), HomeLocation is
 *  the spawn point patrols are centred on.
 */
UCLASS()
class BLACKKNIGHT_API ABKEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	static const FName TargetActorKey;
	static const FName HomeLocationKey;
	static const FName PatrolLocationKey;

protected:
	UPROPERTY(VisibleDefaultsOnly, Category = "Black Knight")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	/** Seconds the enemy keeps chasing a target's last known position after losing sight of it. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|AI", Meta = (ClampMin = 0, Units = "s"))
	float LoseTargetDelay{4.0f};

	FTimerHandle LoseTargetTimerHandle;

public:
	ABKEnemyAIController();

	virtual void Tick(float DeltaTime) override;

	AActor* GetTarget() const;

protected:
	virtual void OnPossess(APawn* InPawn) override;

	virtual void OnUnPossess() override;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void SetTarget(AActor* NewTarget);

	void ClearTarget();
};
