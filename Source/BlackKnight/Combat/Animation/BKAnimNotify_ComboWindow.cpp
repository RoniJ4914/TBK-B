// The Black Knight: Beginnings

#include "BKAnimNotify_ComboWindow.h"

#include "BKMeleeComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

void UBKAnimNotify_ComboWindow::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	const auto* Owner{IsValid(MeshComp) ? MeshComp->GetOwner() : nullptr};
	if (auto* Melee{IsValid(Owner) ? Owner->FindComponentByClass<UBKMeleeComponent>() : nullptr}; IsValid(Melee))
	{
		Melee->CheckCombo(Animation);
	}
}

FString UBKAnimNotify_ComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("BK Combo Window");
}
