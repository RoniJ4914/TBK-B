// The Black Knight: Beginnings

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BKDamageable.generated.h"

/**
 *  Shared damage contract for the player and enemy characters, so both sides
 *  of combat (and the shared attack-trace code) can deal with either through
 *  one interface rather than casting to concrete classes.
 */
UINTERFACE(MinimalAPI, NotBlueprintable)
class UBKDamageable : public UInterface
{
	GENERATED_BODY()
};

class IBKDamageable
{
	GENERATED_BODY()

public:
	/** Applies damage and knockback. Implementers are responsible for their own invulnerability/block checks. */
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& HitLocation, const FVector& HitImpulse) = 0;

	/** Interrupts and staggers this actor, e.g. as a Perfect Block punish. */
	virtual void ApplyStagger(float Duration, AActor* StaggerInstigator) = 0;

	/** Whether this actor can currently be harmed (false during i-frames, after death, etc). */
	virtual bool IsDamageable() const = 0;

	/** Whether this actor is still alive. */
	virtual bool IsAlive() const = 0;
};
