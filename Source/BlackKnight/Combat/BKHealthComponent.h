// The Black Knight: Beginnings

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BKHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBKOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBKOnDeath);

/**
 *  Shared HP pool. Owning actors (player, enemies) are responsible for their
 *  own damage modifiers (blocking, invulnerability) before calling Damage();
 *  this component only owns the number and death/change notifications.
 */
UCLASS(ClassGroup = ("Black Knight"), Meta = (BlueprintSpawnableComponent))
class BLACKKNIGHT_API UBKHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBKHealthComponent();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health", Meta = (ClampMin = 0))
	float MaxHealth{100.0f};

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Health")
	float CurrentHealth{0.0f};

	bool bIsDead{false};

public:
	UPROPERTY(BlueprintAssignable, Category = "Health")
	FBKOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FBKOnDeath OnDeath;

protected:
	virtual void BeginPlay() override;

public:
	/** Applies damage, clamped to zero, and fires OnDeath the first time it reaches zero. Returns true if this call killed the actor. */
	UFUNCTION(BlueprintCallable, Category = "Health")
	bool ApplyDamage(float Damage);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ResetHealth();

	/** For subclasses tuning their pool in their constructor; takes effect at BeginPlay (or immediately via ResetHealth). */
	void SetMaxHealth(const float NewMaxHealth) { MaxHealth = FMath::Max(1.0f, NewMaxHealth); }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsAlive() const { return !bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }
};
