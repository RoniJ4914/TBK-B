// The Black Knight: Beginnings

#pragma once

#include "Blueprint/UserWidget.h"
#include "BKHUDWidget.generated.h"

class ABKPlayerCharacter;
class UImage;
class UProgressBar;
class UTextBlock;

/**
 *  Player HUD: health bar (with a trailing "damage taken" segment), stamina
 *  bar, and centre-screen combat feedback (Perfect Block, Guard Broken, death).
 *
 *  The widget tree is built in C++ so the HUD works with no Widget Blueprint;
 *  it reads the possessed ABKPlayerCharacter every frame, so respawns and
 *  possession changes need no rebinding beyond the feedback events.
 */
UCLASS()
class BLACKKNIGHT_API UBKHUDWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthTrailBar;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> StaminaBar;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> FeedbackText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> FeedbackFlash;

	TWeakObjectPtr<ABKPlayerCharacter> BoundPlayer;
	FDelegateHandle PerfectBlockHandle;
	FDelegateHandle GuardBrokenHandle;

	float TrailPercent{1.0f};
	float TrailHoldTime{0.0f};
	float FeedbackTimeRemaining{0.0f};

	static constexpr float FeedbackDuration{0.9f};

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual void NativeDestruct() override;

	void BindToPlayer(ABKPlayerCharacter* Player);

	void ShowFeedback(const FText& Text, const FLinearColor& Color);

	void OnPerfectBlock();

	void OnGuardBroken();
};
