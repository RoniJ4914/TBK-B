// The Black Knight: Beginnings

#pragma once

#include "AlsCharacter.h"
#include "BKDamageable.h"
#include "BKMeleeComponent.h"
#include "BKCombatCharacter.generated.h"

class UBKHealthComponent;
class UCameraShakeBase;
class UNiagaraSystem;

/**
 *  Shared base for everything that fights: the player and all enemies.
 *
 *  Owns what both sides need identically - the ALS placeholder art wiring,
 *  health, melee combos, the IBKDamageable contract, stagger, and death (ALS
 *  ragdoll) - so a new enemy type only adds its own AI and tuning.
 *
 *  Combat state is expressed through ALS's LocomotionAction tag slot (see
 *  BKCombatTags.h), which makes ALS suppress jumping/mantling/rolling while
 *  attacking or staggered.
 */
UCLASS(Abstract, AutoExpandCategories = ("Settings|Black Knight"))
class BLACKKNIGHT_API ABKCombatCharacter : public AAlsCharacter, public IBKDamageable
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Black Knight")
	TObjectPtr<UBKHealthComponent> Health;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Black Knight")
	TObjectPtr<UBKMeleeComponent> Melee;

	/** Spawned where this character is hit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Feedback")
	TObjectPtr<UNiagaraSystem> HitEffect;

	/** Played on this character's own camera (if player-controlled) when it takes damage. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Feedback")
	TSubclassOf<UCameraShakeBase> HitTakenCameraShake;

	/** Played on the attacker's camera (if player-controlled) when it damages this character. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Feedback")
	TSubclassOf<UCameraShakeBase> HitDealtCameraShake;

	FTimerHandle StaggerTimerHandle;

public:
	explicit ABKCombatCharacter(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;

	/** Attacks, blocks and rolls all need the character alive, grounded and not already mid-action. */
	bool CanStartCombatAction() const;

	/** Starts a combo, or buffers the next step if that combo is already playing. Returns true only when a new combo starts. */
	bool TryStartAttack(EBKAttackType Type);

	/** Starts a combo that continues for NumSteps without further input (AI). */
	bool TryStartAttackChain(EBKAttackType Type, int32 NumSteps);

	void OnMeleeAttackEnded(bool bInterrupted);

	void EndStagger();

	/** Hit spark at Location; bEmphasized (e.g. a Perfect Block) makes it bigger. */
	void PlayHitEffects(const FVector& Location, bool bEmphasized = false) const;

	static void PlayCameraShake(const AActor* Actor, TSubclassOf<UCameraShakeBase> Shake);

	/** Bound to the health component's OnDeath. Ragdolls via ALS; subclasses add their own cleanup. */
	UFUNCTION()
	virtual void HandleDeath();

public:
	UBKHealthComponent* GetHealth() const { return Health; }

	UBKMeleeComponent* GetMelee() const { return Melee; }

	// ~begin IBKDamageable interface
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& HitLocation, const FVector& HitImpulse) override;
	virtual void ApplyStagger(float Duration, AActor* StaggerInstigator) override;
	virtual bool IsDamageable() const override;
	virtual bool IsAlive() const override;
	// ~end IBKDamageable interface
};
