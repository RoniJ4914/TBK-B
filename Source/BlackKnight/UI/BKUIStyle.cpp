// The Black Knight: Beginnings

#include "BKUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

namespace BKUIStyle
{
	UProgressBar* MakeBar(UWidgetTree& Tree, const FLinearColor& Fill, const FLinearColor& Background)
	{
		auto* Bar{Tree.ConstructWidget<UProgressBar>()};

		FProgressBarStyle Style{Bar->GetWidgetStyle()};
		Style.BackgroundImage = FSlateColorBrush{Background};
		Style.FillImage = FSlateColorBrush{FLinearColor::White};
		Style.MarqueeImage = FSlateColorBrush{FLinearColor::Transparent};
		Bar->SetWidgetStyle(Style);
		Bar->SetFillColorAndOpacity(Fill);
		Bar->SetPercent(1.0f);

		return Bar;
	}

	UTextBlock* MakeText(UWidgetTree& Tree, const int32 FontSize, const FLinearColor& Color)
	{
		auto* Text{Tree.ConstructWidget<UTextBlock>()};

		FSlateFontInfo Font{Text->GetFont()};
		Font.Size = FontSize;
		Font.OutlineSettings.OutlineSize = 2;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(Color);
		Text->SetJustification(ETextJustify::Center);

		return Text;
	}
}
