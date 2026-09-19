// The Black Knight: Beginnings

#include "BKHealthComponent.h"

UBKHealthComponent::UBKHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UBKHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	ResetHealth();
}

bool UBKHealthComponent::ApplyDamage(const float Damage)
{
	if (bIsDead || Damage <= 0.0f)
	{
		return false;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);

	if (CurrentHealth <= 0.0f)
	{
		bIsDead = true;
		OnDeath.Broadcast();
		return true;
	}

	return false;
}

void UBKHealthComponent::Heal(const float Amount)
{
	if (bIsDead || Amount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

void UBKHealthComponent::ResetHealth()
{
	bIsDead = false;
	CurrentHealth = MaxHealth;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}
