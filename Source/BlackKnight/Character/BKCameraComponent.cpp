// The Black Knight: Beginnings

#include "BKCameraComponent.h"

#include "AlsCameraSettings.h"
#include "BKDamageable.h"
#include "Animation/AnimInstance.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "UObject/ConstructorHelpers.h"

UBKCameraComponent::UBKCameraComponent()
{
	// The ALS camera is a skeletal mesh component: it reads camera lag, pivot and
	// FOV values from curves on its own animation instance, so all three of
	// mesh, animation class and settings must be present or BeginPlay will ensure.

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CameraMesh{
		TEXT("/ALS/ALSCamera/SKM_Als_Camera.SKM_Als_Camera")
	};

	static ConstructorHelpers::FClassFinder<UAnimInstance> CameraAnimation{
		TEXT("/ALS/ALSCamera/AB_Als_Camera")
	};

	static ConstructorHelpers::FObjectFinder<UAlsCameraSettings> CameraSettings{
		TEXT("/ALS/ALSCamera/Data/CS_Als_Default.CS_Als_Default")
	};

	if (CameraMesh.Succeeded())
	{
		SetSkeletalMeshAsset(CameraMesh.Object);
	}

	if (CameraAnimation.Succeeded())
	{
		SetAnimInstanceClass(CameraAnimation.Class);
	}

	if (CameraSettings.Succeeded())
	{
		Settings = CameraSettings.Object;
	}
}

void UBKCameraComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	UpdateLockOn(DeltaTime);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

bool UBKCameraComponent::TryLockOn()
{
	const auto* Owner{GetOwner()};
	const auto* Pawn{Cast<APawn>(Owner)};
	const auto* Controller{IsValid(Pawn) ? Pawn->GetController() : nullptr};
	if (!IsValid(Controller))
	{
		return false;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ViewDirection{ViewRotation.Vector()};

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams{SCENE_QUERY_STAT(BKLockOnOverlap), false, Owner};
	GetWorld()->OverlapMultiByObjectType(Overlaps, Owner->GetActorLocation(), FQuat::Identity,
	                                     FCollisionObjectQueryParams{ECC_Pawn},
	                                     FCollisionShape::MakeSphere(LockOnRange), QueryParams);

	AActor* BestTarget{nullptr};
	float BestScore{TNumericLimits<float>::Max()};
	const float MinDot{FMath::Cos(FMath::DegreesToRadians(LockOnMaxAngle))};

	for (const FOverlapResult& Overlap : Overlaps)
	{
		auto* Candidate{Overlap.GetActor()};
		const auto* Damageable{Cast<IBKDamageable>(Candidate)};
		if (Damageable == nullptr || !Damageable->IsAlive())
		{
			continue;
		}

		const FVector ToCandidate{Candidate->GetActorLocation() - ViewLocation};
		const float Dot{static_cast<float>(FVector::DotProduct(ToCandidate.GetSafeNormal(), ViewDirection))};
		if (Dot < MinDot || !HasLineOfSightTo(Candidate, ViewLocation))
		{
			continue;
		}

		// Favor whatever is closest to the center of the screen; distance only breaks near-ties.
		const float AngleDegrees{FMath::RadiansToDegrees(FMath::Acos(Dot))};
		const float Score{AngleDegrees + static_cast<float>(ToCandidate.Size()) / LockOnRange * 5.0f};
		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}

	if (BestTarget == nullptr)
	{
		return false;
	}

	SetLockOnTarget(BestTarget);
	return true;
}

void UBKCameraComponent::ClearLockOn()
{
	SetLockOnTarget(nullptr);
}

void UBKCameraComponent::SetLockOnTarget(AActor* NewTarget)
{
	if (LockOnTarget.Get() == NewTarget)
	{
		return;
	}

	LockOnTarget = NewTarget;
	LockOnLostSightTime = 0.0f;
	OnLockOnTargetChanged.Broadcast(NewTarget);
}

void UBKCameraComponent::UpdateLockOn(const float DeltaTime)
{
	if (!LockOnTarget.IsValid())
	{
		// Target was destroyed without going through ClearLockOn.
		if (!LockOnTarget.IsExplicitlyNull())
		{
			ClearLockOn();
		}
		return;
	}

	const auto* Target{LockOnTarget.Get()};
	const auto* Owner{GetOwner()};
	const auto* Pawn{Cast<APawn>(Owner)};
	auto* Controller{IsValid(Pawn) ? Pawn->GetController() : nullptr};
	const auto* Damageable{Cast<IBKDamageable>(Target)};

	if (!IsValid(Controller) || Damageable == nullptr || !Damageable->IsAlive() ||
	    FVector::DistSquared(Owner->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(LockOnBreakRange))
	{
		ClearLockOn();
		return;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	Controller->GetPlayerViewPoint(ViewLocation, ViewRotation);

	LockOnLostSightTime = HasLineOfSightTo(Target, ViewLocation) ? 0.0f : LockOnLostSightTime + DeltaTime;
	if (LockOnLostSightTime > LockOnLostSightGrace)
	{
		ClearLockOn();
		return;
	}

	// Aim from the character, not the camera, so the lock doesn't feed back into the
	// camera's own orbit offset.
	FRotator DesiredRotation{(Target->GetActorLocation() - Owner->GetActorLocation()).Rotation()};
	DesiredRotation.Pitch = FMath::ClampAngle(DesiredRotation.Pitch + LockOnPitchOffset, -60.0f, 45.0f);
	DesiredRotation.Roll = 0.0f;

	Controller->SetControlRotation(FMath::RInterpTo(Controller->GetControlRotation(), DesiredRotation, DeltaTime, LockOnRotationInterpSpeed));
}

bool UBKCameraComponent::HasLineOfSightTo(const AActor* Target, const FVector& From) const
{
	FCollisionQueryParams QueryParams{SCENE_QUERY_STAT(BKLockOnSight), false, GetOwner()};
	QueryParams.AddIgnoredActor(Target);

	return !GetWorld()->LineTraceTestByChannel(From, Target->GetActorLocation(), ECC_Visibility, QueryParams);
}
