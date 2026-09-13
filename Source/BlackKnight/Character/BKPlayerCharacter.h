// The Black Knight: Beginnings

#pragma once

#include "AlsCharacter.h"
#include "BKPlayerCharacter.generated.h"

struct FInputActionValue;
class UBKCameraComponent;
class UInputMappingContext;
class UInputAction;

/**
 *  Player character for The Black Knight.
 *
 *  Derives from AAlsCharacter, so locomotion, stance, gait, rotation modes,
 *  mantling, rolling and ragdolling come from ALS. This class owns the
 *  project-side concerns: camera, Enhanced Input bindings, and (from
 *  Milestone 2 onward) combat state.
 *
 *  All ALS asset defaults are wired in the constructor so the class is
 *  playable without a Blueprint subclass; a Blueprint child can override
 *  any of them for art passes.
 */
UCLASS(AutoExpandCategories = ("Settings|Black Knight"))
class BLACKKNIGHT_API ABKPlayerCharacter : public AAlsCharacter
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Black Knight")
	TObjectPtr<UBKCameraComponent> Camera;

	// Input

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> InputMappingContext;

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

	/** Bound to ALS grounded rolling. Milestone 2 replaces this with the i-frame dodge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Black Knight|Input", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RollAction;

	// Look sensitivity

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Camera", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookUpMouseSensitivity{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Camera", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookRightMouseSensitivity{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Camera", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float LookUpRate{90.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Black Knight|Camera", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float LookRightRate{240.0f};

public:
	explicit ABKPlayerCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void NotifyControllerChanged() override;

protected:
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

public:
	virtual void DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& Unused, float& VerticalLocation) override;
};
