// The Black Knight: Beginnings

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BKStaminaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBKOnStaminaChanged, float, NewStamina, float, MaxStamina);

/**
 *  Stamina resource for the player's dodge/block moveset. Regen pauses for a
 *  short delay after any spend, then ramps back up over time - Souls-like,
 *  not an instant refill.
 */
UCLASS(ClassGroup = ("Black Knight"), Meta = (BlueprintSpawnableComponent))
class BLACKKNIGHT_API UBKStaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBKStaminaComponent();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", Meta = (ClampMin = 0))
	float MaxStamina{100.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stamina")
	float CurrentStamina{0.0f};

	/** Stamina points regenerated per second once regen resumes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", Meta = (ClampMin = 0))
	float RegenRate{35.0f};

	/** Seconds after the last spend before regen resumes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stamina", Meta = (ClampMin = 0, Units = "s"))
	float RegenDelay{1.0f};

	float TimeSinceLastSpend{0.0f};

public:
	UPROPERTY(BlueprintAssignable, Category = "Stamina")
	FBKOnStaminaChanged OnStaminaChanged;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool HasStamina(float Amount) const { return CurrentStamina >= Amount; }

	/** All-or-nothing spend for discrete actions (roll). Fails without side effects if insufficient. */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	bool TryConsumeStamina(float Amount);

	/** Continuous drain for held actions (block). Clamps at zero instead of failing. */
	UFUNCTION(BlueprintCallable, Category = "Stamina")
	void DrainStamina(float Amount);

	UFUNCTION(BlueprintPure, Category = "Stamina")
	bool IsDepleted() const { return CurrentStamina <= 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetCurrentStamina() const { return CurrentStamina; }

	UFUNCTION(BlueprintPure, Category = "Stamina")
	float GetMaxStamina() const { return MaxStamina; }

protected:
	void MarkSpent();
};
