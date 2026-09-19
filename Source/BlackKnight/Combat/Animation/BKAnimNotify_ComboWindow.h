// The Black Knight: Beginnings

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "BKAnimNotify_ComboWindow.generated.h"

/**
 *  Placed near the end of each light-attack combo section: tells the owner's
 *  UBKMeleeComponent to continue into the next section if light attack input
 *  was buffered.
 */
UCLASS(DisplayName = "BK Combo Window")
class BLACKKNIGHT_API UBKAnimNotify_ComboWindow : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
