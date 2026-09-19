// The Black Knight: Beginnings

#pragma once

#include "BKCombatCharacter.h"
#include "BKPlayerCharacter.generated.h"

struct FInputActionValue;
class UAnimMontage;
class UBKCameraComponent;
class UBKStaminaComponent;
class UInputMappingContext;
class UInputAction;

DECLARE_MULTICAST_DELEGATE(FBKOnPlayerCombatEvent);

/**
 *  Player character for The Black Knight.
 *
 *  Locomotion (stance, gait, rotation modes, mantling, rolling, ragdolling)
 *  comes from ALS; health, melee combos, stagger and death come from
 *  ABKCombatCharacter. This class adds what only the player has: camera and
 *  lock-on, Enhanced Input bindings, stamina, the i-frame roll, and
 *  block/Perfect Block.
 *
 *  All asset defaults are wired in C++ constructors so the class is playable
 *  without a Blueprint subclass; a Blueprint child can override any of them.
 */
UCLASS(AutoExpandCategories = ("Settings|Black Knight"))
class BLACKKNIGHT_API ABKPlayerCharacter : public ABKCombatCharacter
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Black Knight")
	TObjectPtr<UBKCameraComponent> Camera;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Black Knight")
	TObjectPtr<UBKStaminaComponent> Stamina;

	// Input

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> InputMappingContext;

	/** Additive mapping context for Black Knight-specific combat actions, kept separate from ALS's own IMC. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> CombatInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookMouseAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> CrouchAction;

	/** Dodge roll, combat-aware: costs stamina and grants a brief i-frame window. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RollAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> BlockAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LightAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> HeavyAttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LockOnAction;

	// Look sensitivity

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Camera", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookUpMouseSensitivity{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Camera", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookRightMouseSensitivity{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Camera", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float LookUpRate{90.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Camera", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float LookRightRate{240.0f};

	// Sprint

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Stamina", Meta = (ClampMin = 0))
	float SprintStaminaPerSecond{18.0f};

	/** After sprinting runs stamina dry, sprint stays locked out until stamina recovers to this much. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Stamina", Meta = (ClampMin = 0))
	float SprintResumeStamina{25.0f};

	bool bWantsToSprint{false};
	bool bSprintExhausted{false};

	// Roll / i-frames

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Combat|Roll", Meta = (ClampMin = 0))
	float RollStaminaCost{25.0f};

	/** How long after a roll starts the character ignores incoming damage. Tuned to the front portion of ALS's roll montage, not its full length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Combat|Roll", Meta = (ClampMin = 0, Units = "s"))
	float RollInvulnerabilityDuration{0.5f};

	// Block / Perfect Block

	/** Guard animation, looped on the arms while Block is held. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Combat|Block")
	TObjectPtr<UAnimMontage> BlockMontage;

	/** Stamina drained per second just for holding the guard up (hits cost extra, see BlockStaminaCostPerDamage). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Combat|Block", Meta = (ClampMin = 0))
	float BlockStaminaPerSecond{6.0f};

	/** Fraction of incoming damage still applied while blocking (not perfectly timed). 0 = full block, 1 = no mitigation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Combat|Block", Meta = (ClampMin = 0, ClampMax = 1))
	float BlockDamageMitigation{0.75f};

	/** Stamina drained per point of raw (pre-mitigation) damage absorbed by a normal block. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Combat|Block", Meta = (ClampMin = 0))
	float BlockStaminaCostPerDamage{2.0f};

	/** Window from the moment Block is pressed during which a hit counts as a Perfect Block instead of a normal block. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Combat|Block", Meta = (ClampMin = 0, Units = "s"))
	float PerfectBlockWindow{0.2f};

	/** How long a Perfect Block staggers the attacker for. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Combat|Block", Meta = (ClampMin = 0, Units = "s"))
	float PerfectBlockStaggerDuration{1.0f};

	bool bIsInvulnerable{false};
	bool bIsBlocking{false};
	bool bInPerfectBlockWindow{false};

	FTimerHandle InvulnerabilityTimerHandle;
	FTimerHandle PerfectBlockWindowTimerHandle;

	/** Rotation mode to restore when lock-on ends (lock-on forces ViewDirection so the character strafes facing the target). */
	FGameplayTag RotationModeBeforeLockOn;

public:
	/** A hit was Perfect Blocked (HUD feedback). */
	FBKOnPlayerCombatEvent OnPerfectBlock;

	/** Stamina ran out while blocking and the guard collapsed (HUD feedback). */
	FBKOnPlayerCombatEvent OnGuardBroken;

	explicit ABKPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void NotifyControllerChanged() override;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& ViewInfo) override;

	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

	// Input handlers

	virtual void Input_OnMove(const FInputActionValue& ActionValue);

	virtual void Input_OnLookMouse(const FInputActionValue& ActionValue);

	virtual void Input_OnLook(const FInputActionValue& ActionValue);

	virtual void Input_OnJump(const FInputActionValue& ActionValue);

	virtual void Input_OnSprint(const FInputActionValue& ActionValue);

	virtual void Input_OnWalk();

	virtual void Input_OnCrouch();

	virtual void Input_OnRoll();

	virtual void Input_OnBlockStart();

	virtual void Input_OnBlockEnd();

	virtual void Input_OnLightAttack();

	virtual void Input_OnHeavyAttack();

	virtual void Input_OnLockOn();

	void OnLockOnTargetChanged(AActor* NewTarget);

	void BeginInvulnerabilityWindow(float Duration);
	void EndInvulnerabilityWindow();

	void EndPerfectBlockWindow();

	void BreakGuard();

	/** Applies the desired gait from sprint input, stamina, and exhaustion. */
	void RefreshSprintGait();

	virtual void HandleDeath() override;

public:
	UFUNCTION(BlueprintPure, Category = "Black Knight|Combat")
	bool IsInvulnerable() const { return bIsInvulnerable; }

	UFUNCTION(BlueprintPure, Category = "Black Knight|Combat")
	bool IsBlocking() const { return bIsBlocking; }

	UBKCameraComponent* GetBKCamera() const { return Camera; }

	UBKStaminaComponent* GetStamina() const { return Stamina; }

	/** Development self-test hooks: act as if the player pressed the button. */
	bool DebugTryAttack(const EBKAttackType Type) { return TryStartAttack(Type); }
	void DebugSetBlocking(const bool bBlock) { bBlock ? Input_OnBlockStart() : Input_OnBlockEnd(); }

	// ~begin IBKDamageable interface
	virtual void ApplyDamage(float Damage, AActor* DamageCauser, const FVector& HitLocation, const FVector& HitImpulse) override;
	virtual void ApplyStagger(float Duration, AActor* StaggerInstigator) override;
	virtual bool IsDamageable() const override;
	// ~end IBKDamageable interface

	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& Unused, float& VerticalLocation) override;
};
