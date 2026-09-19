// The Black Knight: Beginnings

#include "BKCheatManager.h"

#include "BlackKnight.h"
#include "BKCameraComponent.h"
#include "BKDamageable.h"
#include "AIController.h"
#include "BKEnemyCharacter.h"
#include "BKPlayerCharacter.h"
#include "BKStaminaComponent.h"
#include "BrainComponent.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "TimerManager.h"

namespace BKCombatSelfTestSettings
{
	constexpr float TickInterval{0.1f};
	constexpr double Timeout{90.0};
	constexpr double AttackDistance{220.0};
}

void UBKCheatManager::BKCombatSelfTest(const FString& Mode)
{
	SelfTestMode = Mode;

	const auto* Player{Cast<ABKPlayerCharacter>(GetPlayerController()->GetPawn())};
	if (!IsValid(Player))
	{
		UE_LOG(LogBlackKnight, Error, TEXT("BKCombatSelfTest: FAILED - no ABKPlayerCharacter possessed."));
		return;
	}

	SelfTestStartTime = GetWorld()->GetTimeSeconds();
	SelfTestCombosStarted = 0;
	SelfTestTarget.Reset();

	GetWorld()->GetTimerManager().SetTimer(SelfTestTimerHandle, this, &ThisClass::TickCombatSelfTest,
	                                       BKCombatSelfTestSettings::TickInterval, true);

	UE_LOG(LogBlackKnight, Display, TEXT("BKCombatSelfTest: started (%s)."), *SelfTestMode);
}

void UBKCheatManager::TickCombatSelfTest()
{
	auto* Player{Cast<ABKPlayerCharacter>(GetPlayerController()->GetPawn())};
	const double Elapsed{GetWorld()->GetTimeSeconds() - SelfTestStartTime};

	if (!IsValid(Player) || !Player->IsAlive())
	{
		FinishCombatSelfTest(false, TEXT("player died"));
		return;
	}

	// The lock-on breaks as soon as its target dies, so remember who we were fighting.
	if (const auto* Target{Cast<IBKDamageable>(SelfTestTarget.Get())}; Target != nullptr && !Target->IsAlive())
	{
		FinishCombatSelfTest(true, FString::Printf(TEXT("killed %s"), *SelfTestTarget->GetName()));
		return;
	}

	if (Elapsed > BKCombatSelfTestSettings::Timeout)
	{
		FinishCombatSelfTest(false, TEXT("timed out"));
		return;
	}

	auto* Camera{Player->GetBKCamera()};
	if (!IsValid(Camera->GetLockOnTarget()) && Camera->TryLockOn())
	{
		SelfTestTarget = Camera->GetLockOnTarget();
		UE_LOG(LogBlackKnight, Display, TEXT("BKCombatSelfTest: locked on to %s at %.1fs."), *SelfTestTarget->GetName(), Elapsed);
	}

	const auto* Target{Camera->GetLockOnTarget()};
	if (!IsValid(Target) || FVector::Dist2D(Player->GetActorLocation(), Target->GetActorLocation()) > BKCombatSelfTestSettings::AttackDistance)
	{
		// Wait for the enemy to come to us; the self-test doesn't steer the player.
		return;
	}

	// Mash the combo so every step chains.
	const bool bHeavy{
		SelfTestMode.Equals(TEXT("Heavy"), ESearchCase::IgnoreCase) ||
		(!SelfTestMode.Equals(TEXT("Light"), ESearchCase::IgnoreCase) && SelfTestCombosStarted % 3 == 2)
	};
	const EBKAttackType Type{bHeavy ? EBKAttackType::Heavy : EBKAttackType::Light};
	const EBKAttackType Buffered{Player->GetMelee()->IsAttacking() ? Player->GetMelee()->GetCurrentAttack() : Type};
	if (Player->DebugTryAttack(Buffered))
	{
		++SelfTestCombosStarted;
		UE_LOG(LogBlackKnight, Display, TEXT("BKCombatSelfTest: started %s combo at %.1fs."),
		       Type == EBKAttackType::Heavy ? TEXT("heavy") : TEXT("light"), Elapsed);
	}
}

void UBKCheatManager::BKDebugScreenshot(const FString& Mode, const float YawOffset)
{
	auto* Controller{GetPlayerController()};
	auto* Player{Cast<ABKPlayerCharacter>(Controller->GetPawn())};
	if (!IsValid(Player))
	{
		UE_LOG(LogBlackKnight, Error, TEXT("BKDebugScreenshot: no ABKPlayerCharacter possessed."));
		return;
	}

	ABKEnemyCharacter* Enemy{nullptr};
	for (TActorIterator<ABKEnemyCharacter> It{GetWorld()}; It; ++It)
	{
		Enemy = *It;
		if (auto* AIController{Cast<AAIController>(Enemy->GetController())}; IsValid(AIController) && IsValid(AIController->GetBrainComponent()))
		{
			AIController->GetBrainComponent()->StopLogic(TEXT("BKDebugScreenshot"));
		}
	}

	Controller->SetControlRotation(FRotator{-8.0, Player->GetActorRotation().Yaw + 180.0 + YawOffset, 0.0});

	double CaptureDelay{1.5};
	if (Mode.Equals(TEXT("Block"), ESearchCase::IgnoreCase))
	{
		Player->DebugSetBlocking(true);
	}
	else if (Mode.Equals(TEXT("Attack"), ESearchCase::IgnoreCase))
	{
		// Let the camera settle, then capture the first light swing at its impact frame.
		constexpr double ImpactTime{0.46};
		FTimerHandle AttackTimer;
		GetWorld()->GetTimerManager().SetTimer(AttackTimer, [PlayerWeak = TWeakObjectPtr<ABKPlayerCharacter>{Player}]
		{
			if (auto* AttackingPlayer{PlayerWeak.Get()}; IsValid(AttackingPlayer))
			{
				AttackingPlayer->DebugTryAttack(EBKAttackType::Light);
			}
		}, 1.2f, false);
		CaptureDelay = 1.2 + ImpactTime;
	}
	else if (Mode.Equals(TEXT("Hurt"), ESearchCase::IgnoreCase))
	{
		Player->ApplyDamage(35.0f, Enemy, Player->GetActorLocation(), FVector::ZeroVector);
		if (IsValid(Enemy))
		{
			Enemy->ApplyDamage(20.0f, Player, Enemy->GetActorLocation(), FVector::ZeroVector);
		}
		Player->GetStamina()->DrainStamina(40.0f);
		CaptureDelay = 0.3;
	}

	const FString Filename{FPaths::ConvertRelativePathToFull(FPaths::ScreenShotDir() / FString::Printf(TEXT("BK_%s.png"), *Mode))};
	GetWorld()->GetTimerManager().SetTimer(ScreenshotTimerHandle,
	                                       [Filename, PlayerWeak = TWeakObjectPtr<ABKPlayerCharacter>{Player}]
	{
		// Animation-layer diagnostics: which montage slots ALS is actually blending, and the
		// layering curves that gate them.
		if (const auto* ShotPlayer{PlayerWeak.Get()}; IsValid(ShotPlayer))
		{
			if (const auto* AnimInstance{ShotPlayer->GetMesh()->GetAnimInstance()}; IsValid(AnimInstance))
			{
				for (const FName SlotName : {TEXT("ArmLeft"), TEXT("ArmRight"), TEXT("Curves"), TEXT("PostLocomotion")})
				{
					UE_LOG(LogBlackKnight, Display, TEXT("BKDebugScreenshot: slot %s montage weight %.2f"),
					       *SlotName.ToString(), AnimInstance->GetSlotMontageGlobalWeight(SlotName));
				}

				for (const FName CurveName : {TEXT("LayerArmLeftSlot"), TEXT("LayerArmRightSlot"), TEXT("LayerArmLeft"), TEXT("LayerArmLeftLocalSpace")})
				{
					UE_LOG(LogBlackKnight, Display, TEXT("BKDebugScreenshot: curve %s = %.2f"),
					       *CurveName.ToString(), AnimInstance->GetCurveValue(CurveName));
				}
			}
		}

		FScreenshotRequest::RequestScreenshot(Filename, true, false);
		UE_LOG(LogBlackKnight, Display, TEXT("BKDebugScreenshot: requested %s"), *Filename);
	}, static_cast<float>(CaptureDelay), false);
}

void UBKCheatManager::FinishCombatSelfTest(const bool bPassed, const FString& Reason)
{
	GetWorld()->GetTimerManager().ClearTimer(SelfTestTimerHandle);

	const double Elapsed{GetWorld()->GetTimeSeconds() - SelfTestStartTime};
	if (bPassed)
	{
		UE_LOG(LogBlackKnight, Display, TEXT("BKCombatSelfTest: PASSED - %s in %.1fs (%d combos)."), *Reason, Elapsed, SelfTestCombosStarted);
	}
	else
	{
		UE_LOG(LogBlackKnight, Error, TEXT("BKCombatSelfTest: FAILED - %s after %.1fs (%d combos)."), *Reason, Elapsed, SelfTestCombosStarted);
	}
}
