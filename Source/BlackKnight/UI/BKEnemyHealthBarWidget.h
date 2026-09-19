// The Black Knight: Beginnings

#pragma once

#include "Blueprint/UserWidget.h"
#include "BKEnemyHealthBarWidget.generated.h"

class ABKCombatCharacter;
class UProgressBar;

/**
 *  Small health bar shown over an enemy's head (via a screen-space widget
 *  component). Stays hidden until the enemy has taken damage, and hides again
 *  on death. Tree built in C++; no Widget Blueprint needed.
 */
UCLASS()
class BLACKKNIGHT_API UBKEnemyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthBar;

	TWeakObjectPtr<ABKCombatCharacter> Owner;

public:
	void SetOwnerCharacter(ABKCombatCharacter* NewOwner) { Owner = NewOwner; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};
