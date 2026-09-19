// The Black Knight: Beginnings

#include "BKHUDWidget.h"

#include "BKHealthComponent.h"
#include "BKPlayerCharacter.h"
#include "BKStaminaComponent.h"
#include "BKUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "BKHUD"

namespace BKHUDLayout
{
	constexpr float HealthWidth{380.0f};
	constexpr float HealthHeight{16.0f};
	constexpr float StaminaWidth{300.0f};
	constexpr float StaminaHeight{10.0f};
	constexpr float ScreenMargin{48.0f};

	/** How long the damage-taken trail waits before draining down to the real health. */
	constexpr float TrailHoldSeconds{0.6f};
	constexpr float TrailDrainPerSecond{0.6f};
}

TSharedRef<SWidget> UBKHUDWidget::RebuildWidget()
{
	if (IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		auto* Root{WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"))};
		WidgetTree->RootWidget = Root;

		// Full-screen tint that flashes on combat feedback.
		FeedbackFlash = WidgetTree->ConstructWidget<UImage>();
		FeedbackFlash->SetBrush(FSlateColorBrush{FLinearColor::White});
		FeedbackFlash->SetColorAndOpacity(FLinearColor::Transparent);
		FeedbackFlash->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (auto* FlashSlot{Root->AddChildToCanvas(FeedbackFlash)})
		{
			FlashSlot->SetAnchors(FAnchors{0.0f, 0.0f, 1.0f, 1.0f});
			FlashSlot->SetOffsets(FMargin{0.0f});
		}

		// Bars, bottom-left.
		auto* Bars{WidgetTree->ConstructWidget<UVerticalBox>()};
		if (auto* BarsSlot{Root->AddChildToCanvas(Bars)})
		{
			BarsSlot->SetAnchors(FAnchors{0.0f, 1.0f});
			BarsSlot->SetAlignment(FVector2D{0.0f, 1.0f});
			BarsSlot->SetPosition(FVector2D{BKHUDLayout::ScreenMargin, -BKHUDLayout::ScreenMargin});
			BarsSlot->SetAutoSize(true);
		}

		auto* HealthBox{WidgetTree->ConstructWidget<USizeBox>()};
		HealthBox->SetWidthOverride(BKHUDLayout::HealthWidth);
		HealthBox->SetHeightOverride(BKHUDLayout::HealthHeight);
		Bars->AddChildToVerticalBox(HealthBox);

		auto* HealthStack{WidgetTree->ConstructWidget<UOverlay>()};
		HealthBox->AddChild(HealthStack);

		// Overlay slots default to auto-size, which collapses a progress bar to nothing.
		const auto StretchToFill{[](UOverlaySlot* OverlaySlot)
		{
			if (OverlaySlot != nullptr)
			{
				OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
				OverlaySlot->SetVerticalAlignment(VAlign_Fill);
			}
		}};

		HealthTrailBar = BKUIStyle::MakeBar(*WidgetTree, BKUIStyle::HealthTrail);
		StretchToFill(HealthStack->AddChildToOverlay(HealthTrailBar));
		HealthBar = BKUIStyle::MakeBar(*WidgetTree, BKUIStyle::HealthFill, FLinearColor::Transparent);
		StretchToFill(HealthStack->AddChildToOverlay(HealthBar));

		auto* StaminaBox{WidgetTree->ConstructWidget<USizeBox>()};
		StaminaBox->SetWidthOverride(BKHUDLayout::StaminaWidth);
		StaminaBox->SetHeightOverride(BKHUDLayout::StaminaHeight);
		if (auto* StaminaSlot{Bars->AddChildToVerticalBox(StaminaBox)})
		{
			StaminaSlot->SetPadding(FMargin{0.0f, 6.0f, 0.0f, 0.0f});
		}

		StaminaBar = BKUIStyle::MakeBar(*WidgetTree, BKUIStyle::StaminaFill);
		StaminaBox->AddChild(StaminaBar);

		// Centre feedback text.
		FeedbackText = BKUIStyle::MakeText(*WidgetTree, 40, FLinearColor::White);
		FeedbackText->SetRenderOpacity(0.0f);
		if (auto* TextSlot{Root->AddChildToCanvas(FeedbackText)})
		{
			TextSlot->SetAnchors(FAnchors{0.5f, 0.35f});
			TextSlot->SetAlignment(FVector2D{0.5f, 0.5f});
			TextSlot->SetAutoSize(true);
		}
	}

	return Super::RebuildWidget();
}

void UBKHUDWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const auto* Controller{GetOwningPlayer()};
	auto* Player{IsValid(Controller) ? Cast<ABKPlayerCharacter>(Controller->GetPawn()) : nullptr};
	if (Player != BoundPlayer.Get())
	{
		BindToPlayer(Player);
	}

	if (!IsValid(Player))
	{
		return;
	}

	const auto* Health{Player->GetHealth()};
	const float HealthPercent{Health->GetMaxHealth() > 0.0f ? Health->GetCurrentHealth() / Health->GetMaxHealth() : 0.0f};
	HealthBar->SetPercent(HealthPercent);

	// The trail jumps up instantly (healing) but lingers, then drains, after damage.
	if (HealthPercent >= TrailPercent)
	{
		TrailPercent = HealthPercent;
		TrailHoldTime = BKHUDLayout::TrailHoldSeconds;
	}
	else if ((TrailHoldTime -= InDeltaTime) <= 0.0f)
	{
		TrailPercent = FMath::Max(HealthPercent, TrailPercent - BKHUDLayout::TrailDrainPerSecond * InDeltaTime);
	}
	HealthTrailBar->SetPercent(TrailPercent);

	const auto* Stamina{Player->GetStamina()};
	StaminaBar->SetPercent(Stamina->GetMaxStamina() > 0.0f ? Stamina->GetCurrentStamina() / Stamina->GetMaxStamina() : 0.0f);
	StaminaBar->SetFillColorAndOpacity(Stamina->IsDepleted() ? BKUIStyle::StaminaExhausted : BKUIStyle::StaminaFill);

	if (!Player->IsAlive())
	{
		FeedbackText->SetText(LOCTEXT("Died", "YOU DIED"));
		FeedbackText->SetColorAndOpacity(BKUIStyle::Death);
		FeedbackText->SetRenderOpacity(1.0f);
		FeedbackFlash->SetColorAndOpacity(FLinearColor{0.0f, 0.0f, 0.0f, 0.35f});
		return;
	}

	if (FeedbackTimeRemaining > 0.0f)
	{
		FeedbackTimeRemaining = FMath::Max(0.0f, FeedbackTimeRemaining - InDeltaTime);
		const float Alpha{FeedbackTimeRemaining / FeedbackDuration};

		FeedbackText->SetRenderOpacity(FMath::Clamp(Alpha * 2.0f, 0.0f, 1.0f));

		FLinearColor Flash{FeedbackFlash->GetColorAndOpacity()};
		Flash.A = 0.25f * Alpha * Alpha;
		FeedbackFlash->SetColorAndOpacity(Flash);
	}
}

void UBKHUDWidget::NativeDestruct()
{
	BindToPlayer(nullptr);

	Super::NativeDestruct();
}

void UBKHUDWidget::BindToPlayer(ABKPlayerCharacter* Player)
{
	if (auto* Previous{BoundPlayer.Get()}; IsValid(Previous))
	{
		Previous->OnPerfectBlock.Remove(PerfectBlockHandle);
		Previous->OnGuardBroken.Remove(GuardBrokenHandle);
	}

	BoundPlayer = Player;
	PerfectBlockHandle.Reset();
	GuardBrokenHandle.Reset();

	if (IsValid(Player))
	{
		PerfectBlockHandle = Player->OnPerfectBlock.AddUObject(this, &ThisClass::OnPerfectBlock);
		GuardBrokenHandle = Player->OnGuardBroken.AddUObject(this, &ThisClass::OnGuardBroken);

		TrailPercent = 1.0f;
		FeedbackTimeRemaining = 0.0f;
		FeedbackText->SetRenderOpacity(0.0f);
		FeedbackFlash->SetColorAndOpacity(FLinearColor::Transparent);
	}
}

void UBKHUDWidget::ShowFeedback(const FText& Text, const FLinearColor& Color)
{
	FeedbackText->SetText(Text);
	FeedbackText->SetColorAndOpacity(Color);
	FeedbackFlash->SetColorAndOpacity(FLinearColor{Color.R, Color.G, Color.B, 0.0f});
	FeedbackTimeRemaining = FeedbackDuration;
}

void UBKHUDWidget::OnPerfectBlock()
{
	ShowFeedback(LOCTEXT("PerfectBlock", "PERFECT BLOCK"), BKUIStyle::PerfectBlock);
}

void UBKHUDWidget::OnGuardBroken()
{
	ShowFeedback(LOCTEXT("GuardBroken", "GUARD BROKEN"), BKUIStyle::GuardBroken);
}

#undef LOCTEXT_NAMESPACE
