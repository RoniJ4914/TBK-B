// The Black Knight: Beginnings

#pragma once

#include "CoreMinimal.h"

class UProgressBar;
class UTextBlock;
class UWidgetTree;

/**
 *  Placeholder UI look shared by the C++-built widgets: flat, untextured bars
 *  and outlined text, so the HUD reads clearly without any art assets.
 */
namespace BKUIStyle
{
	const FLinearColor HealthFill{0.62f, 0.05f, 0.05f};
	const FLinearColor HealthTrail{0.95f, 0.78f, 0.35f, 0.85f};
	const FLinearColor StaminaFill{0.24f, 0.62f, 0.26f};
	const FLinearColor StaminaExhausted{0.45f, 0.45f, 0.45f};
	const FLinearColor BarBackground{0.0f, 0.0f, 0.0f, 0.6f};
	const FLinearColor PerfectBlock{0.55f, 0.8f, 1.0f};
	const FLinearColor GuardBroken{1.0f, 0.55f, 0.15f};
	const FLinearColor Death{0.55f, 0.02f, 0.02f};

	/** A flat progress bar. Pass a transparent Background to stack bars in an overlay. */
	UProgressBar* MakeBar(UWidgetTree& Tree, const FLinearColor& Fill, const FLinearColor& Background = BarBackground);

	UTextBlock* MakeText(UWidgetTree& Tree, int32 FontSize, const FLinearColor& Color);
}
