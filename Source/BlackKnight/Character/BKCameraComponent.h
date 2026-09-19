// The Black Knight: Beginnings

#pragma once

#include "AlsCameraComponent.h"
#include "BKCameraComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FBKOnLockOnTargetChanged, AActor* /*NewTarget*/);

/**
 *  Project camera for the player character.
 *
 *  Subclasses UAlsCameraComponent so it can:
 *   1. supply its default settings asset from C++ (Settings is protected), and
 *   2. own lock-on targeting. While locked on, it steers the controller's
 *      control rotation toward the target; ALS's camera already follows
 *      control rotation, so the regular camera rig (lag, pivot, collision)
 *      keeps working unchanged.
 */
UCLASS(ClassGroup = "BlackKnight", Meta = (BlueprintSpawnableComponent))
class BLACKKNIGHT_API UBKCameraComponent : public UAlsCameraComponent
{
	GENERATED_BODY()

protected:
	/** Max distance to acquire a lock-on target. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Lock-On", Meta = (ClampMin = 0, Units = "cm"))
	float LockOnRange{1500.0f};

	/** Lock breaks when the target gets farther than this. Kept above LockOnRange so it doesn't flicker at the edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Lock-On", Meta = (ClampMin = 0, Units = "cm"))
	float LockOnBreakRange{2000.0f};

	/** Targets must be within this angle of the camera's forward direction to be acquired. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Lock-On", Meta = (ClampMin = 0, ClampMax = 180, Units = "deg"))
	float LockOnMaxAngle{40.0f};

	/** Lock breaks after the target has been out of line of sight this long. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Lock-On", Meta = (ClampMin = 0, Units = "s"))
	float LockOnLostSightGrace{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Lock-On", Meta = (ClampMin = 0))
	float LockOnRotationInterpSpeed{8.0f};

	/** Added to the pitch toward the target so the camera looks slightly down over the character rather than straight at the target's center. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Lock-On", Meta = (ClampMin = -90, ClampMax = 90, Units = "deg"))
	float LockOnPitchOffset{-12.0f};

	TWeakObjectPtr<AActor> LockOnTarget;

	float LockOnLostSightTime{0.0f};

public:
	FBKOnLockOnTargetChanged OnLockOnTargetChanged;

public:
	UBKCameraComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Locks onto the best living IBKDamageable target in range and view. Returns false if none qualifies. */
	bool TryLockOn();

	void ClearLockOn();

	AActor* GetLockOnTarget() const { return LockOnTarget.Get(); }

protected:
	void UpdateLockOn(float DeltaTime);

	bool HasLineOfSightTo(const AActor* Target, const FVector& From) const;

	void SetLockOnTarget(AActor* NewTarget);
};
