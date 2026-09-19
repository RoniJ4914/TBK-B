// The Black Knight: Beginnings

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "BKAnimNotify_AttackTrace.generated.h"

/**
 *  Placed at the impact frame of an attack montage: tells the owner's
 *  UBKMeleeComponent to sweep for and damage targets in front of SourceBone.
 */
UCLASS(DisplayName = "BK Attack Trace")
class BLACKKNIGHT_API UBKAnimNotify_AttackTrace : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attack")
	FName SourceBone;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
