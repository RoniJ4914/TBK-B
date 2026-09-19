// The Black Knight: Beginnings

#pragma once

#include "Commandlets/Commandlet.h"
#include "BKCreateCombatInputAssetsCommandlet.generated.h"

/**
 *  Editor-only commandlet that creates the Black Knight-authored combat Input
 *  Actions and their Input Mapping Context under /Game/Input, kept separate
 *  from ALS's own IMC so combat bindings can evolve without touching plugin
 *  content. Additive and idempotent: existing assets are loaded, missing
 *  actions/mappings are added, nothing is removed. Warns on keys shared with
 *  ALS's IMC.
 *
 *  Run with: UnrealEditor-Cmd.exe <uproject> -run=BKCreateCombatInputAssets
 */
UCLASS()
class UBKCreateCombatInputAssetsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	virtual int32 Main(const FString& Params) override;
};
