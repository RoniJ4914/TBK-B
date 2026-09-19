// The Black Knight: Beginnings

#include "BKCreateEnemyAICommandlet.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "BKEnemyAIController.h"
#include "BTTask_BKFindPatrolLocation.h"
#include "BTTask_BKMeleeAttack.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* const BlackboardPath{TEXT("/Game/AI/BB_BK_Enemy")};
	const TCHAR* const BehaviorTreePath{TEXT("/Game/AI/BT_BK_Enemy")};

	UPackage* PrepareFreshPackage(const FString& PackagePath, const FString& AssetName)
	{
		UPackage* Package{CreatePackage(*PackagePath)};
		Package->FullyLoad();

		if (auto* Existing{FindObject<UObject>(Package, *AssetName)}; IsValid(Existing))
		{
			Existing->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
			Existing->MarkAsGarbage();
		}

		return Package;
	}

	bool SaveAsset(UPackage* Package, UObject* Asset)
	{
		Package->MarkPackageDirty();
		FAssetRegistryModule::AssetCreated(Asset);

		const FString PackageFileName{FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension())};

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;

		const bool bSaved{UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs)};
		if (!bSaved)
		{
			UE_LOG(LogTemp, Error, TEXT("BKCreateEnemyAI: failed to save %s."), *Package->GetName());
		}
		return bSaved;
	}

	/** Most node settings are protected UPROPERTYs meant for the details panel; set them the same way the editor does. */
	void SetProperty(UObject* Object, const TCHAR* PropertyName, const TCHAR* Value)
	{
		const FProperty* Property{FindFProperty<FProperty>(Object->GetClass(), PropertyName)};
		if (Property == nullptr || Property->ImportText_InContainer(Value, Object, Object, PPF_None) == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("BKCreateEnemyAI: could not set %s.%s = %s"), *Object->GetClass()->GetName(), PropertyName, Value);
		}
	}

	/** Sets only the key name, keeping the node's own key-type filters intact (importing the whole struct would wipe them). */
	void SetKey(UObject* Node, const FName KeyName, const TCHAR* PropertyName = TEXT("BlackboardKey"))
	{
		const auto* Property{FindFProperty<FStructProperty>(Node->GetClass(), PropertyName)};
		if (Property == nullptr || Property->Struct != FBlackboardKeySelector::StaticStruct())
		{
			UE_LOG(LogTemp, Error, TEXT("BKCreateEnemyAI: %s has no key selector %s"), *Node->GetClass()->GetName(), PropertyName);
			return;
		}

		Property->ContainerPtrToValuePtr<FBlackboardKeySelector>(Node)->SelectedKeyName = KeyName;
	}

	template <typename T>
	T* NewNode(UBehaviorTree* Tree, const TCHAR* Name)
	{
		auto* Node{NewObject<T>(Tree)};
		Node->NodeName = Name;
		return Node;
	}

	void AddTask(UBTCompositeNode* Parent, UBTTaskNode* Task)
	{
		Parent->Children.AddDefaulted_GetRef().ChildTask = Task;
	}

	UBTTask_MoveTo* NewMoveTo(UBehaviorTree* Tree, const TCHAR* Name, const FName Key, const float AcceptableRadius)
	{
		auto* MoveTo{NewNode<UBTTask_MoveTo>(Tree, Name)};
		SetKey(MoveTo, Key);
		SetProperty(MoveTo, TEXT("AcceptableRadius"), *FString::Printf(TEXT("(DefaultValue=%f)"), AcceptableRadius));
		return MoveTo;
	}

	UBTTask_Wait* NewWait(UBehaviorTree* Tree, const TCHAR* Name, const float Time, const float Deviation)
	{
		auto* Wait{NewNode<UBTTask_Wait>(Tree, Name)};
		SetProperty(Wait, TEXT("WaitTime"), *FString::Printf(TEXT("(DefaultValue=%f)"), Time));
		SetProperty(Wait, TEXT("RandomDeviation"), *FString::Printf(TEXT("(DefaultValue=%f)"), Deviation));
		return Wait;
	}
}

int32 UBKCreateEnemyAICommandlet::Main(const FString& Params)
{
	// Blackboard

	UPackage* BlackboardPackage{PrepareFreshPackage(BlackboardPath, TEXT("BB_BK_Enemy"))};
	auto* Blackboard{NewObject<UBlackboardData>(BlackboardPackage, TEXT("BB_BK_Enemy"), RF_Public | RF_Standalone)};

	const auto AddKey{[Blackboard](const FName Name, UBlackboardKeyType* KeyType)
	{
		FBlackboardEntry& Entry{Blackboard->Keys.AddDefaulted_GetRef()};
		Entry.EntryName = Name;
		Entry.KeyType = KeyType;
	}};

	auto* TargetKeyType{NewObject<UBlackboardKeyType_Object>(Blackboard)};
	TargetKeyType->BaseClass = AActor::StaticClass();
	AddKey(ABKEnemyAIController::TargetActorKey, TargetKeyType);
	AddKey(ABKEnemyAIController::HomeLocationKey, NewObject<UBlackboardKeyType_Vector>(Blackboard));
	AddKey(ABKEnemyAIController::PatrolLocationKey, NewObject<UBlackboardKeyType_Vector>(Blackboard));
	Blackboard->UpdateKeyIDs();

	if (!SaveAsset(BlackboardPackage, Blackboard))
	{
		return 1;
	}

	// Behavior Tree

	UPackage* TreePackage{PrepareFreshPackage(BehaviorTreePath, TEXT("BT_BK_Enemy"))};
	auto* Tree{NewObject<UBehaviorTree>(TreePackage, TEXT("BT_BK_Enemy"), RF_Public | RF_Standalone)};
	Tree->BlackboardAsset = Blackboard;

	auto* Root{NewNode<UBTComposite_Selector>(Tree, TEXT("Root"))};
	Tree->RootNode = Root;

	// Combat branch: runs while there's a target. "Both" aborts patrol the moment a target
	// appears and aborts combat (cancelling any swing) the moment it's lost.
	auto* Combat{NewNode<UBTComposite_Sequence>(Tree, TEXT("Combat"))};
	auto* HasTarget{NewNode<UBTDecorator_Blackboard>(Tree, TEXT("Has Target"))};
	SetKey(HasTarget, ABKEnemyAIController::TargetActorKey);
	SetProperty(HasTarget, TEXT("BasicOperation"), TEXT("Set"));
	SetProperty(HasTarget, TEXT("OperationType"), TEXT("0"));
	SetProperty(HasTarget, TEXT("FlowAbortMode"), TEXT("Both"));
	SetProperty(HasTarget, TEXT("NotifyObserver"), TEXT("ResultChange"));

	FBTCompositeChild& CombatChild{Root->Children.AddDefaulted_GetRef()};
	CombatChild.ChildComposite = Combat;
	CombatChild.Decorators.Add(HasTarget);

	AddTask(Combat, NewMoveTo(Tree, TEXT("Chase Target"), ABKEnemyAIController::TargetActorKey, 60.0f));

	auto* Attack{NewNode<UBTTask_BKMeleeAttack>(Tree, TEXT("Attack Target"))};
	SetKey(Attack, ABKEnemyAIController::TargetActorKey);
	AddTask(Combat, Attack);

	AddTask(Combat, NewWait(Tree, TEXT("Attack Cooldown"), 0.8f, 0.4f));

	// Patrol branch.
	auto* Patrol{NewNode<UBTComposite_Sequence>(Tree, TEXT("Patrol"))};
	Root->Children.AddDefaulted_GetRef().ChildComposite = Patrol;

	auto* FindPatrol{NewNode<UBTTask_BKFindPatrolLocation>(Tree, TEXT("Find Patrol Location"))};
	SetKey(FindPatrol, ABKEnemyAIController::PatrolLocationKey);
	SetKey(FindPatrol, ABKEnemyAIController::HomeLocationKey, TEXT("HomeKey"));
	AddTask(Patrol, FindPatrol);

	AddTask(Patrol, NewMoveTo(Tree, TEXT("Walk To Patrol Location"), ABKEnemyAIController::PatrolLocationKey, 50.0f));
	AddTask(Patrol, NewWait(Tree, TEXT("Patrol Pause"), 3.0f, 1.0f));

	if (!SaveAsset(TreePackage, Tree))
	{
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("BKCreateEnemyAI: saved %s and %s"), BlackboardPath, BehaviorTreePath);
	return 0;
}
#else
int32 UBKCreateEnemyAICommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Error, TEXT("BKCreateEnemyAI: editor-only commandlet, WITH_EDITOR=0."));
	return 1;
}
#endif
