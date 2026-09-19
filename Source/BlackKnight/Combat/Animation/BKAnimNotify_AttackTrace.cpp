// The Black Knight: Beginnings

#include "BKAnimNotify_AttackTrace.h"

#include "BKMeleeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UBKAnimNotify_AttackTrace::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	const auto* Owner{IsValid(MeshComp) ? MeshComp->GetOwner() : nullptr};
	if (auto* Melee{IsValid(Owner) ? Owner->FindComponentByClass<UBKMeleeComponent>() : nullptr}; IsValid(Melee))
	{
		Melee->DoAttackTrace(SourceBone, Animation);
	}
}

FString UBKAnimNotify_AttackTrace::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("BK Attack Trace (%s)"), *SourceBone.ToString());
}
