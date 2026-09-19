// The Black Knight: Beginnings

#include "BKHUD.h"

#include "BKCameraComponent.h"
#include "BKHUDWidget.h"
#include "BKPlayerCharacter.h"
#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"

void ABKHUD::BeginPlay()
{
	Super::BeginPlay();

	auto* Controller{GetOwningPlayerController()};
	if (IsValid(Controller) && Controller->IsLocalController())
	{
		HUDWidget = CreateWidget<UBKHUDWidget>(Controller, UBKHUDWidget::StaticClass(), TEXT("BKHUD"));
		if (IsValid(HUDWidget))
		{
			HUDWidget->AddToViewport();
		}
	}
}

void ABKHUD::DrawHUD()
{
	Super::DrawHUD();

	const auto* Controller{GetOwningPlayerController()};
	const auto* Player{IsValid(Controller) ? Cast<ABKPlayerCharacter>(Controller->GetPawn()) : nullptr};
	const auto* Target{IsValid(Player) ? Player->GetBKCamera()->GetLockOnTarget() : nullptr};
	if (!IsValid(Target) || Canvas == nullptr)
	{
		return;
	}

	// Mark the target's chest: a small diamond outline around a dot.
	const FVector ScreenPoint{Project(Target->GetActorLocation() + FVector{0.0, 0.0, 30.0}, true)};
	if (ScreenPoint.Z <= 0.0)
	{
		return;
	}

	static constexpr float Radius{11.0f};
	const FLinearColor Color{1.0f, 1.0f, 1.0f, 0.9f};
	const FVector2D Center{ScreenPoint.X, ScreenPoint.Y};
	const FVector2D Points[]{{0.0, -Radius}, {Radius, 0.0}, {0.0, Radius}, {-Radius, 0.0}};

	for (int32 Index{0}; Index < UE_ARRAY_COUNT(Points); ++Index)
	{
		const FVector2D Start{Center + Points[Index]};
		const FVector2D End{Center + Points[(Index + 1) % UE_ARRAY_COUNT(Points)]};
		DrawLine(Start.X, Start.Y, End.X, End.Y, Color, 2.0f);
	}

	DrawRect(Color, Center.X - 2.0f, Center.Y - 2.0f, 4.0f, 4.0f);
}
