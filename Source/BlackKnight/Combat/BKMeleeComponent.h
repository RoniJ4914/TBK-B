// The Black Knight: Beginnings

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BKMeleeComponent.generated.h"

class ACharacter;
class UAnimInstance;
class UAnimMontage;
class UAnimSequenceBase;
class UBKStaminaComponent;

UENUM(BlueprintType)
enum class EBKAttackType : uint8
{
	None,
	Light,
	Heavy
};

USTRUCT(BlueprintType)
struct BLACKKNIGHT_API FBKAttackStep
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	TObjectPtr<UAnimMontage> Montage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", Meta = (ClampMin = 0))
	float Damage{10.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack", Meta = (ClampMin = 0))
	float StaminaCost{10.0f};
};

DECLARE_MULTICAST_DELEGATE_OneParam(FBKOnAttackEnded, bool /*bInterrupted*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FBKOnAttackStepStarted, EBKAttackType /*Type*/, int32 /*StepIndex*/);

/**
 *  Montage-driven melee combos shared by the player and enemies.
 *
 *  Light and heavy attacks are each a chain of steps, one montage per step.
 *  Pressing the same attack again shortly before a step's ComboWindow notify
 *  continues into the next step, which cross-fades in (each montage blends in
 *  and out, so there is no pose pop between steps or back to idle). Hit timing
 *  comes from AttackTrace notifies in each montage.
 *
 *  If the owner has a UBKStaminaComponent, each step costs stamina and is
 *  refused without it; owners without one (enemies) attack for free.
 */
UCLASS(ClassGroup = ("Black Knight"), Meta = (BlueprintSpawnableComponent))
class BLACKKNIGHT_API UBKMeleeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBKMeleeComponent();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee|Combos")
	TArray<FBKAttackStep> LightCombo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee|Combos")
	TArray<FBKAttackStep> HeavyCombo;

	/** How long before a ComboWindow notify an attack input still counts toward continuing the combo. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee|Combos", Meta = (ClampMin = 0, Units = "s"))
	float ComboInputBufferTime{0.45f};

	/** Scales every step's damage, so enemies can share the player's combo data at their own strength. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee", Meta = (ClampMin = 0))
	float DamageMultiplier{1.0f};

	/** Distance the hit sphere sweeps forward from the notify's source bone. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee|Trace", Meta = (ClampMin = 0, Units = "cm"))
	float TraceDistance{75.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee|Trace", Meta = (ClampMin = 0, Units = "cm"))
	float TraceRadius{60.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Melee|Trace", Meta = (ClampMin = 0, Units = "cm/s"))
	float KnockbackImpulse{250.0f};

	UPROPERTY(Transient)
	TObjectPtr<UBKStaminaComponent> Stamina;

	EBKAttackType CurrentAttack{EBKAttackType::None};
	int32 ComboIndex{0};
	double LastAttackInputTime{-1000.0};

	/** World time the current step's montage started playing; negative while not attacking. */
	double StepStartTime{-1.0};

	/** Set once the current step's AttackTrace notify has fired: the swing has landed and is no longer steerable. */
	bool bStepImpactPassed{false};

	/** Steps still to chain without input (AI-driven combos). */
	int32 QueuedChainSteps{0};

	/** Incremented per step so a stale montage callback from a previous step can't end the current one. */
	int32 AttackSerial{0};

	TArray<TWeakObjectPtr<AActor>> ActorsHitThisStep;

public:
	FBKOnAttackEnded OnAttackEnded;

	/** Each time a step's montage starts, including the first step of a combo. */
	FBKOnAttackStepStarted OnAttackStepStarted;

protected:
	virtual void BeginPlay() override;

public:
	/** Starts the Type combo, or buffers a continuation if that combo is already playing. Returns true only when a new combo starts. */
	bool StartAttack(EBKAttackType Type);

	/** Starts the Type combo and continues it for up to NumSteps steps with no further input. For AI. */
	bool StartAttackChain(EBKAttackType Type, int32 NumSteps);

	/** Interrupts the current attack (stagger, death). OnAttackEnded fires with bInterrupted = true. */
	void CancelAttack();

	void SetDamageMultiplier(const float NewMultiplier) { DamageMultiplier = FMath::Max(0.0f, NewMultiplier); }

	UFUNCTION(BlueprintPure, Category = "Melee")
	bool IsAttacking() const { return CurrentAttack != EBKAttackType::None; }

	UFUNCTION(BlueprintPure, Category = "Melee")
	EBKAttackType GetCurrentAttack() const { return CurrentAttack; }

	int32 GetComboIndex() const { return ComboIndex; }

	int32 GetComboLength(EBKAttackType Type) const { return GetCombo(Type).Num(); }

	/** Seconds since the current step's montage started, or a large number while not attacking. */
	float GetTimeSinceStepStarted() const;

	/** True once this step's hit frame has passed, so the swing is committed. */
	bool HasStepImpactPassed() const { return bStepImpactPassed; }

	/**
	 * Called by UBKAnimNotify_AttackTrace. Damages each IBKDamageable in front of SourceBone at most once per step.
	 * Ignored unless Animation is the current step's montage (a previous step still fading out keeps firing notifies).
	 */
	void DoAttackTrace(FName SourceBone, const UAnimSequenceBase* Animation);

	/** Called by UBKAnimNotify_ComboWindow. Continues into the next step if input was buffered or a chain is queued. */
	void CheckCombo(const UAnimSequenceBase* Animation);

protected:
	const TArray<FBKAttackStep>& GetCombo(EBKAttackType Type) const;

	const FBKAttackStep* GetCurrentStep() const;

	bool PlayStep(EBKAttackType Type, int32 Index);

	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted, int32 Serial);

	UAnimInstance* GetOwnerAnimInstance() const;
};
