// The Black Knight: Beginnings

#include "BKCombatCharacter.h"

#include "BlackKnight.h"
#include "BKCombatTags.h"
#include "BKHealthComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraShakeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/SkeletalMesh.h"
#include "Settings/AlsCharacterSettings.h"
#include "Settings/AlsMovementSettings.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Utility/AlsGameplayTags.h"

namespace BKCharacterAssets
{
	// ALS plugin content, mounted at /ALS/. Kept together so the art pass has a
	// single place to look when swapping in final Black Knight assets.
	const TCHAR* const CharacterMesh{TEXT("/ALS/ALS/Character/SKM_Als.SKM_Als")};
	const TCHAR* const CharacterAnimation{TEXT("/ALS/ALS/Character/AB_Als")};
	const TCHAR* const CharacterSettings{TEXT("/ALS/ALS/Data/Character/CS_Als_Default.CS_Als_Default")};
	const TCHAR* const MovementSettings{TEXT("/ALS/ALS/Data/Character/Movement/MS_Als_Normal.MS_Als_Normal")};

	// Placeholder combat feedback from the original template's combat variant.
	const TCHAR* const HitEffect{TEXT("/Game/Variant_Combat/VFX/NS_Damage.NS_Damage")};
	const TCHAR* const HitTakenShake{TEXT("/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Player")};
	const TCHAR* const HitDealtShake{TEXT("/Game/Variant_Combat/Blueprints/BP_CameraShake_Hit_Enemy")};
}

ABKCombatCharacter::ABKCombatCharacter(const FObjectInitializer& ObjectInitializer) : Super{ObjectInitializer}
{
	Health = CreateDefaultSubobject<UBKHealthComponent>(TEXT("Health"));
	Melee = CreateDefaultSubobject<UBKMeleeComponent>(TEXT("Melee"));

	// Placeholder character art. The ALS base constructor has already set the capsule
	// size and the mesh offset/rotation that this skeleton expects.

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset{BKCharacterAssets::CharacterMesh};
	if (MeshAsset.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MeshAsset.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimationAsset{BKCharacterAssets::CharacterAnimation};
	if (AnimationAsset.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(AnimationAsset.Class);
	}

	static ConstructorHelpers::FObjectFinder<UAlsCharacterSettings> SettingsAsset{BKCharacterAssets::CharacterSettings};
	if (SettingsAsset.Succeeded())
	{
		Settings = SettingsAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAlsMovementSettings> MovementSettingsAsset{BKCharacterAssets::MovementSettings};
	if (MovementSettingsAsset.Succeeded())
	{
		MovementSettings = MovementSettingsAsset.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> HitEffectAsset{BKCharacterAssets::HitEffect};
	static ConstructorHelpers::FClassFinder<UCameraShakeBase> HitTakenShakeClass{BKCharacterAssets::HitTakenShake};
	static ConstructorHelpers::FClassFinder<UCameraShakeBase> HitDealtShakeClass{BKCharacterAssets::HitDealtShake};
	HitEffect = HitEffectAsset.Object;
	HitTakenCameraShake = HitTakenShakeClass.Class;
	HitDealtCameraShake = HitDealtShakeClass.Class;
}

void ABKCombatCharacter::BeginPlay()
{
	Super::BeginPlay();

	Melee->OnAttackEnded.AddUObject(this, &ThisClass::OnMeleeAttackEnded);
	Health->OnDeath.AddDynamic(this, &ThisClass::HandleDeath);
}

bool ABKCombatCharacter::CanStartCombatAction() const
{
	// Mirrors AAlsCharacter::IsRollingAllowedToStart: any LocomotionAction (rolling, mantling,
	// ragdolling, blocking, attacking, staggered...) means the character is already committed.
	return IsAlive() && !GetLocomotionAction().IsValid() && GetLocomotionMode() == AlsLocomotionModeTags::Grounded;
}

bool ABKCombatCharacter::TryStartAttack(const EBKAttackType Type)
{
	if (!IsAlive())
	{
		return false;
	}

	if (Melee->IsAttacking())
	{
		// Buffers the next step of the same combo.
		Melee->StartAttack(Type);
		return false;
	}

	if (!CanStartCombatAction() || !Melee->StartAttack(Type))
	{
		return false;
	}

	SetLocomotionAction(BKLocomotionActionTags::Attacking);
	return true;
}

bool ABKCombatCharacter::TryStartAttackChain(const EBKAttackType Type, const int32 NumSteps)
{
	if (!CanStartCombatAction() || !Melee->StartAttackChain(Type, NumSteps))
	{
		return false;
	}

	SetLocomotionAction(BKLocomotionActionTags::Attacking);
	return true;
}

void ABKCombatCharacter::OnMeleeAttackEnded(bool)
{
	if (GetLocomotionAction() == BKLocomotionActionTags::Attacking)
	{
		SetLocomotionAction(FGameplayTag::EmptyTag);
	}
}

void ABKCombatCharacter::ApplyDamage(const float Damage, AActor* DamageCauser, const FVector& HitLocation, const FVector&)
{
	if (!IsDamageable() || Damage <= 0.0f)
	{
		return;
	}

	// Logged before applying, since a lethal hit fires OnDeath (and its log) from inside ApplyDamage.
	UE_LOG(LogBlackKnight, Log, TEXT("%s took %.1f damage from %s (%.0f / %.0f)"), *GetName(), Damage,
	       *GetNameSafe(DamageCauser), FMath::Max(0.0f, Health->GetCurrentHealth() - Damage), Health->GetMaxHealth());

	PlayHitEffects(HitLocation);
	PlayCameraShake(this, HitTakenCameraShake);
	PlayCameraShake(DamageCauser, HitDealtCameraShake);

	Health->ApplyDamage(Damage);
}

void ABKCombatCharacter::PlayHitEffects(const FVector& Location, const bool bEmphasized) const
{
	if (IsValid(HitEffect))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, HitEffect, Location, FRotator::ZeroRotator,
		                                               FVector{bEmphasized ? 2.0 : 1.0});
	}
}

void ABKCombatCharacter::PlayCameraShake(const AActor* Actor, const TSubclassOf<UCameraShakeBase> Shake)
{
	const auto* Pawn{Cast<APawn>(Actor)};
	if (auto* PlayerController{IsValid(Pawn) ? Cast<APlayerController>(Pawn->GetController()) : nullptr};
		IsValid(PlayerController) && Shake != nullptr)
	{
		PlayerController->ClientStartCameraShake(Shake);
	}
}

void ABKCombatCharacter::ApplyStagger(const float Duration, AActor* StaggerInstigator)
{
	if (!IsDamageable())
	{
		return;
	}

	// Locks the character out of actions for Duration. There is no dedicated stagger animation yet.
	Melee->CancelAttack();
	SetLocomotionAction(BKLocomotionActionTags::Staggered);
	GetWorldTimerManager().SetTimer(StaggerTimerHandle, this, &ThisClass::EndStagger, Duration, false);

	UE_LOG(LogBlackKnight, Log, TEXT("%s staggered for %.1fs by %s"), *GetName(), Duration, *GetNameSafe(StaggerInstigator));
}

void ABKCombatCharacter::EndStagger()
{
	if (GetLocomotionAction() == BKLocomotionActionTags::Staggered)
	{
		SetLocomotionAction(FGameplayTag::EmptyTag);
	}
}

void ABKCombatCharacter::HandleDeath()
{
	UE_LOG(LogBlackKnight, Log, TEXT("%s died"), *GetName());

	Melee->CancelAttack();
	GetWorldTimerManager().ClearTimer(StaggerTimerHandle);
	SetLocomotionAction(FGameplayTag::EmptyTag);

	StartRagdolling();
}

bool ABKCombatCharacter::IsDamageable() const
{
	return IsAlive();
}

bool ABKCombatCharacter::IsAlive() const
{
	return Health->IsAlive();
}
