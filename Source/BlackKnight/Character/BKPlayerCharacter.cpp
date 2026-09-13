// The Black Knight: Beginnings

#include "BKPlayerCharacter.h"

#include "BKCameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"
#include "Settings/AlsCharacterSettings.h"
#include "Settings/AlsMovementSettings.h"
#include "UObject/ConstructorHelpers.h"
#include "Utility/AlsVector.h"

namespace BKPlayerCharacterAssets
{
	// ALS plugin content, mounted at /ALS/. Kept together so the art pass has a
	// single place to look when swapping in final Black Knight assets.
	const TCHAR* const CharacterMesh{TEXT("/ALS/ALS/Character/SKM_Als.SKM_Als")};
	const TCHAR* const CharacterAnimation{TEXT("/ALS/ALS/Character/AB_Als")};
	const TCHAR* const CharacterSettings{TEXT("/ALS/ALS/Data/Character/CS_Als_Default.CS_Als_Default")};
	const TCHAR* const MovementSettings{TEXT("/ALS/ALS/Data/Character/Movement/MS_Als_Normal.MS_Als_Normal")};

	const TCHAR* const MappingContext{TEXT("/ALS/ALS/Data/Input/IMC_Als_Default.IMC_Als_Default")};
	const TCHAR* const Move{TEXT("/ALS/ALS/Data/Input/IA_Als_Move.IA_Als_Move")};
	const TCHAR* const LookMouse{TEXT("/ALS/ALS/Data/Input/IA_Als_LookMouse.IA_Als_LookMouse")};
	const TCHAR* const Look{TEXT("/ALS/ALS/Data/Input/IA_Als_Look.IA_Als_Look")};
	const TCHAR* const Jump{TEXT("/ALS/ALS/Data/Input/IA_Als_Jump.IA_Als_Jump")};
	const TCHAR* const Sprint{TEXT("/ALS/ALS/Data/Input/IA_Als_Sprint.IA_Als_Sprint")};
	const TCHAR* const Walk{TEXT("/ALS/ALS/Data/Input/IA_Als_Walk.IA_Als_Walk")};
	const TCHAR* const Crouch{TEXT("/ALS/ALS/Data/Input/IA_Als_Crouch.IA_Als_Crouch")};
	const TCHAR* const Roll{TEXT("/ALS/ALS/Data/Input/IA_Als_Roll.IA_Als_Roll")};
}

ABKPlayerCharacter::ABKPlayerCharacter(const FObjectInitializer& ObjectInitializer) : Super{ObjectInitializer}
{
	Camera = CreateDefaultSubobject<UBKCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetMesh());
	Camera->SetRelativeRotation_Direct({0.0f, 90.0f, 0.0f});

	// Placeholder character art. The base constructor has already set the capsule
	// size and the mesh offset/rotation that this skeleton expects.

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset{BKPlayerCharacterAssets::CharacterMesh};
	if (MeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MeshAsset.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimationAsset{BKPlayerCharacterAssets::CharacterAnimation};
	if (AnimationAsset.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimationAsset.Class);
	}

	static ConstructorHelpers::FObjectFinder<UAlsCharacterSettings> SettingsAsset{BKPlayerCharacterAssets::CharacterSettings};
	if (SettingsAsset.Succeeded())
	{
		Settings = SettingsAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAlsMovementSettings> MovementSettingsAsset{BKPlayerCharacterAssets::MovementSettings};
	if (MovementSettingsAsset.Succeeded())
	{
		MovementSettings = MovementSettingsAsset.Object;
	}

	// Enhanced Input assets.

	static ConstructorHelpers::FObjectFinder<UInputMappingContext> MappingContextAsset{BKPlayerCharacterAssets::MappingContext};
	if (MappingContextAsset.Succeeded())
	{
		InputMappingContext = MappingContextAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> MoveAsset{BKPlayerCharacterAssets::Move};
	static ConstructorHelpers::FObjectFinder<UInputAction> LookMouseAsset{BKPlayerCharacterAssets::LookMouse};
	static ConstructorHelpers::FObjectFinder<UInputAction> LookAsset{BKPlayerCharacterAssets::Look};
	static ConstructorHelpers::FObjectFinder<UInputAction> JumpAsset{BKPlayerCharacterAssets::Jump};
	static ConstructorHelpers::FObjectFinder<UInputAction> SprintAsset{BKPlayerCharacterAssets::Sprint};
	static ConstructorHelpers::FObjectFinder<UInputAction> WalkAsset{BKPlayerCharacterAssets::Walk};
	static ConstructorHelpers::FObjectFinder<UInputAction> CrouchAsset{BKPlayerCharacterAssets::Crouch};
	static ConstructorHelpers::FObjectFinder<UInputAction> RollAsset{BKPlayerCharacterAssets::Roll};

	MoveAction = MoveAsset.Object;
	LookMouseAction = LookMouseAsset.Object;
	LookAction = LookAsset.Object;
	JumpAction = JumpAsset.Object;
	SprintAction = SprintAsset.Object;
	WalkAction = WalkAsset.Object;
	CrouchAction = CrouchAsset.Object;
	RollAction = RollAsset.Object;
}

void ABKPlayerCharacter::NotifyControllerChanged()
{
	const auto* PreviousPlayer{Cast<APlayerController>(PreviousController)};
	if (IsValid(PreviousPlayer))
	{
		auto* InputSubsystem{ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PreviousPlayer->GetLocalPlayer())};
		if (IsValid(InputSubsystem))
		{
			InputSubsystem->RemoveMappingContext(InputMappingContext);
		}
	}

	auto* NewPlayer{Cast<APlayerController>(GetController())};
	if (IsValid(NewPlayer))
	{
		// ALS applies its own look sensitivity, so neutralize the legacy scales.

		NewPlayer->InputYawScale_DEPRECATED = 1.0f;
		NewPlayer->InputPitchScale_DEPRECATED = 1.0f;
		NewPlayer->InputRollScale_DEPRECATED = 1.0f;

		auto* InputSubsystem{ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayer->GetLocalPlayer())};
		if (IsValid(InputSubsystem))
		{
			FModifyContextOptions Options;
			Options.bNotifyUserSettings = true;

			InputSubsystem->AddMappingContext(InputMappingContext, 0, Options);
		}
	}

	Super::NotifyControllerChanged();
}

void ABKPlayerCharacter::CalcCamera(const float DeltaTime, FMinimalViewInfo& ViewInfo)
{
	if (Camera->IsActive())
	{
		Camera->GetViewInfo(ViewInfo);
		return;
	}

	Super::CalcCamera(DeltaTime, ViewInfo);
}

void ABKPlayerCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	auto* EnhancedInput{Cast<UEnhancedInputComponent>(Input)};
	if (!IsValid(EnhancedInput))
	{
		return;
	}

	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnMove);
	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnMove);
	EnhancedInput->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnLookMouse);
	EnhancedInput->BindAction(LookMouseAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnLookMouse);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnLook);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnLook);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnJump);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnJump);
	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnSprint);
	EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnSprint);
	EnhancedInput->BindAction(WalkAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnWalk);
	EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnCrouch);
	EnhancedInput->BindAction(RollAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnRoll);
}

void ABKPlayerCharacter::Input_OnMove(const FInputActionValue& ActionValue)
{
	const auto Value{UAlsVector::ClampMagnitude012D(ActionValue.Get<FVector2D>())};

	auto ViewRotation{GetViewState().Rotation};

	if (IsValid(GetController()))
	{
		// Prefer the exact camera rotation over the interpolated target rotation.

		FVector ViewLocation;
		GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	const auto ForwardDirection{UAlsVector::AngleToDirectionXY(UE_REAL_TO_FLOAT(ViewRotation.Yaw))};
	const auto RightDirection{UAlsVector::PerpendicularCounterClockwiseXY(ForwardDirection)};

	AddMovementInput(ForwardDirection * Value.Y + RightDirection * Value.X);
}

void ABKPlayerCharacter::Input_OnLookMouse(const FInputActionValue& ActionValue)
{
	const FVector2f Value{ActionValue.Get<FVector2D>()};

	AddControllerPitchInput(Value.Y * LookUpMouseSensitivity);
	AddControllerYawInput(Value.X * LookRightMouseSensitivity);
}

void ABKPlayerCharacter::Input_OnLook(const FInputActionValue& ActionValue)
{
	const FVector2f Value{ActionValue.Get<FVector2D>()};

	AddControllerPitchInput(Value.Y * LookUpRate);
	AddControllerYawInput(Value.X * LookRightRate);
}

void ABKPlayerCharacter::Input_OnJump(const FInputActionValue& ActionValue)
{
	if (!ActionValue.Get<bool>())
	{
		StopJumping();
		return;
	}

	if (StopRagdolling())
	{
		return;
	}

	if (StartMantling())
	{
		return;
	}

	if (GetStance() == AlsStanceTags::Crouching)
	{
		SetDesiredStance(AlsStanceTags::Standing);
		return;
	}

	Jump();
}

void ABKPlayerCharacter::Input_OnSprint(const FInputActionValue& ActionValue)
{
	SetDesiredGait(ActionValue.Get<bool>() ? AlsGaitTags::Sprinting : AlsGaitTags::Running);
}

void ABKPlayerCharacter::Input_OnWalk()
{
	if (GetDesiredGait() == AlsGaitTags::Walking)
	{
		SetDesiredGait(AlsGaitTags::Running);
	}
	else if (GetDesiredGait() == AlsGaitTags::Running)
	{
		SetDesiredGait(AlsGaitTags::Walking);
	}
}

void ABKPlayerCharacter::Input_OnCrouch()
{
	if (GetDesiredStance() == AlsStanceTags::Standing)
	{
		SetDesiredStance(AlsStanceTags::Crouching);
	}
	else if (GetDesiredStance() == AlsStanceTags::Crouching)
	{
		SetDesiredStance(AlsStanceTags::Standing);
	}
}

void ABKPlayerCharacter::Input_OnRoll()
{
	static constexpr auto PlayRate{1.3f};

	StartRollingGrounded(PlayRate);
}

void ABKPlayerCharacter::DisplayDebug(UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo,
                                      float& Unused, float& VerticalLocation)
{
	if (Camera->IsActive())
	{
		Camera->DisplayDebug(Canvas, DisplayInfo, VerticalLocation);
	}

	Super::DisplayDebug(Canvas, DisplayInfo, Unused, VerticalLocation);
}
