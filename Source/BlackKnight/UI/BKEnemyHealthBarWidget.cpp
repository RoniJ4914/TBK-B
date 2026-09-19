// The Black Knight: Beginnings

#include "BKEnemyHealthBarWidget.h"

#include "BKCombatCharacter.h"
#include "BKHealthComponent.h"
#include "BKUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"

TSharedRef<SWidget> UBKEnemyHealthBarWidget::RebuildWidget()
{
	if (IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		auto* Box{WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Root"))};
		Box->SetWidthOverride(110.0f);
		Box->SetHeightOverride(8.0f);
		WidgetTree->RootWidget = Box;

		HealthBar = BKUIStyle::MakeBar(*WidgetTree, BKUIStyle::HealthFill);
		Box->AddChild(HealthBar);

		SetVisibility(ESlateVisibility::Hidden);
	}

	return Super::RebuildWidget();
}

void UBKEnemyHealthBarWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const auto* Character{Owner.Get()};
	const auto* Health{IsValid(Character) ? Character->GetHealth() : nullptr};
	if (!IsValid(Health))
	{
		return;
	}

	const float Percent{Health->GetMaxHealth() > 0.0f ? Health->GetCurrentHealth() / Health->GetMaxHealth() : 0.0f};
	HealthBar->SetPercent(Percent);

	const bool bShow{Health->IsAlive() && Percent < 1.0f};
	SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
}
