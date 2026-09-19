// The Black Knight: Beginnings

#include "BKStaminaComponent.h"

UBKStaminaComponent::UBKStaminaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UBKStaminaComponent::BeginPlay()
{
	Super::BeginPlay();

	CurrentStamina = MaxStamina;
	TimeSinceLastSpend = RegenDelay;
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void UBKStaminaComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (CurrentStamina >= MaxStamina)
	{
		return;
	}

	TimeSinceLastSpend += DeltaTime;

	if (TimeSinceLastSpend < RegenDelay)
	{
		return;
	}

	CurrentStamina = FMath::Min(CurrentStamina + RegenRate * DeltaTime, MaxStamina);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

bool UBKStaminaComponent::TryConsumeStamina(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return true;
	}

	if (!HasStamina(Amount))
	{
		return false;
	}

	CurrentStamina -= Amount;
	MarkSpent();
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
	return true;
}

void UBKStaminaComponent::DrainStamina(const float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	CurrentStamina = FMath::Max(CurrentStamina - Amount, 0.0f);
	MarkSpent();
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina);
}

void UBKStaminaComponent::MarkSpent()
{
	TimeSinceLastSpend = 0.0f;
}
