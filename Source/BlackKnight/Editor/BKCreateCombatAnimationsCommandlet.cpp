// The Black Knight: Beginnings

#include "BKCreateCombatAnimationsCommandlet.h"

#if WITH_EDITOR
#include "AnimationRuntime.h"
#include "Algo/Transform.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "BKAnimNotify_AttackTrace.h"
#include "BKAnimNotify_ComboWindow.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimData/CurveIdentifier.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/PackageName.h"
#include "TwoBoneIK.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace
{
	const FName AlsFullBodySlot{TEXT("PostLocomotion")};
	const TCHAR* const AlsStandPosePath{TEXT("/ALS/ALS/Animations/Base/A_Als_Stand_Pose.A_Als_Stand_Pose")};

	const TCHAR* const AlsSkeletonPath{TEXT("/ALS/ALS/Character/SK_Als.SK_Als")};
	const TCHAR* const AlsMeshPath{TEXT("/ALS/ALS/Character/SKM_Als.SKM_Als")};
	const TCHAR* const AlsRollMontagePath{TEXT("/ALS/ALS/Animations/Actions/Roll/AM_Als_Roll.AM_Als_Roll")};
	const TCHAR* const OutputFolder{TEXT("/Game/Combat/Animations")};

	const TCHAR* const TemplateMontagePaths[]{
		TEXT("/Game/Variant_Combat/Anims/AM_ComboAttack.AM_ComboAttack"),
		TEXT("/Game/Variant_Combat/Anims/AM_ChargedAttack.AM_ChargedAttack"),
	};

	/** Gap after the last hit trace at which to open a combo window, for clips the template never gave one. */
	constexpr float DefaultComboWindowDelay{0.12f};

	struct FBKStepSpec
	{
		const TCHAR* MontageName;
		const TCHAR* SourceClip;
		float PlayRate;
		bool bComboWindow;
	};

	// Must match the montage names UBKMeleeComponent loads.
	const FBKStepSpec StepSpecs[]{
		{TEXT("AM_BK_Light_01"), TEXT("MM_Attack_01"), 1.0f, true},
		{TEXT("AM_BK_Light_02"), TEXT("MM_Attack_02"), 1.0f, true},
		{TEXT("AM_BK_Light_03"), TEXT("MM_Attack_01"), 1.2f, true},
		{TEXT("AM_BK_Light_04"), TEXT("MM_Attack_03"), 1.0f, false},
		{TEXT("AM_BK_Heavy_01"), TEXT("MM_Attack_03"), 0.85f, true},
		{TEXT("AM_BK_Heavy_02"), TEXT("MM_Attack_01"), 0.7f, true},
		{TEXT("AM_BK_Heavy_03"), TEXT("MM_ChargedAttack"), 1.0f, false},
	};

	/** What the template montages authored for one source clip, in clip-local time. */
	struct FBKClipTiming
	{
		TObjectPtr<UAnimSequence> Sequence;
		float ClipStart{0.0f};
		float ClipEnd{0.0f};
		TArray<TPair<float, FName>> Traces;
		TOptional<float> ComboWindow;
	};

	void AddAlsActionCurves(IAnimationDataController& Controller, const UAnimSequence* Sequence, const UAnimMontage* AlsRoll);

	UPackage* PrepareFreshPackage(const FString& PackagePath, const FString& AssetName)
	{
		UPackage* Package{CreatePackage(*PackagePath)};
		Package->FullyLoad();

		if (auto* Existing{FindObject<UObject>(Package, *AssetName)}; IsValid(Existing))
		{
			// A previously generated sequence can still be in the root set (animation compression
			// roots them while it builds), and renaming a rooted object asserts.
			if (auto* ExistingSequence{Cast<UAnimSequence>(Existing)})
			{
				ExistingSequence->WaitOnExistingCompression();
			}
			Existing->RemoveFromRoot();

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
			UE_LOG(LogTemp, Error, TEXT("BKCreateCombatAnimations: failed to save %s."), *Package->GetName());
		}
		return bSaved;
	}

	void GatherTemplateTimings(TMap<FName, FBKClipTiming>& OutTimings)
	{
		for (const TCHAR* TemplatePath : TemplateMontagePaths)
		{
			const auto* Template{LoadObject<UAnimMontage>(nullptr, TemplatePath)};
			if (!IsValid(Template) || Template->SlotAnimTracks.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("BKCreateCombatAnimations: template %s missing, skipping."), TemplatePath);
				continue;
			}

			for (const FAnimSegment& Segment : Template->SlotAnimTracks[0].AnimTrack.AnimSegments)
			{
				auto* Sequence{Cast<UAnimSequence>(Segment.GetAnimReference())};
				if (!IsValid(Sequence) || OutTimings.Contains(Sequence->GetFName()))
				{
					continue;
				}

				FBKClipTiming& Timing{OutTimings.Add(Sequence->GetFName())};
				Timing.Sequence = Sequence;
				Timing.ClipStart = Segment.AnimStartTime;
				Timing.ClipEnd = Segment.AnimEndTime;

				for (const FAnimNotifyEvent& Event : Template->Notifies)
				{
					const float MontageTime{Event.GetTime()};
					if (!IsValid(Event.Notify) || MontageTime < Segment.StartPos || MontageTime >= Segment.GetEndPos())
					{
						continue;
					}

					const float ClipTime{Segment.AnimStartTime + (MontageTime - Segment.StartPos) * Segment.AnimPlayRate};
					const FString ClassName{Event.Notify->GetClass()->GetName()};

					if (ClassName == TEXT("AnimNotify_DoAttackTrace"))
					{
						// AttackBoneName is protected on the template class; read it via reflection.
						FName Bone;
						if (const auto* BoneProperty{FindFProperty<FNameProperty>(Event.Notify->GetClass(), TEXT("AttackBoneName"))})
						{
							Bone = BoneProperty->GetPropertyValue_InContainer(Event.Notify.Get());
						}
						Timing.Traces.Emplace(ClipTime, Bone);
					}
					else if (ClassName == TEXT("AnimNotify_CheckCombo"))
					{
						Timing.ComboWindow = ClipTime;
					}
				}

				Timing.Traces.Sort([](const TPair<float, FName>& A, const TPair<float, FName>& B) { return A.Key < B.Key; });
			}
		}
	}

	/**
	 * Bakes Source onto TargetSkeleton. See the class comment for the approach; per bone:
	 *   mapped:   component rotation = (source component rotation delta from its rest pose) * target rest rotation
	 *   unmapped: keeps its rest rotation relative to its (retargeted) parent
	 *   position: target rest offset from the parent, except root/pelvis which carry
	 *             the source's motion scaled by relative hip height
	 */
	UAnimSequence* BakeRetargetedClip(UAnimSequence* Source, USkeleton* TargetSkeleton, USkeletalMesh* PreviewMesh, const UAnimMontage* AlsRoll)
	{
		const FString AssetName{FString::Printf(TEXT("A_BK_%s"), *Source->GetName())};
		const FString PackagePath{FString::Printf(TEXT("%s/Retargeted/%s"), OutputFolder, *AssetName)};

		const FReferenceSkeleton& SourceRef{Source->GetSkeleton()->GetReferenceSkeleton()};
		const FReferenceSkeleton& TargetRef{TargetSkeleton->GetReferenceSkeleton()};
		const int32 SourceBoneCount{SourceRef.GetRawBoneNum()};
		const int32 TargetBoneCount{TargetRef.GetRawBoneNum()};

		const TArrayView<const FTransform> SourceRestLocal{SourceRef.GetRawRefBonePose()};
		const TArrayView<const FTransform> TargetRestLocal{TargetRef.GetRawRefBonePose()};
		TArray<FTransform> SourceRestComponent;
		TArray<FTransform> TargetRestComponent;
		FAnimationRuntime::FillUpComponentSpaceTransforms(SourceRef, SourceRestLocal, SourceRestComponent);
		FAnimationRuntime::FillUpComponentSpaceTransforms(TargetRef, TargetRestLocal, TargetRestComponent);

		// ALS's spine is 3 bones where Manny's is 5: map the top of each spine onto each other
		// so everything hanging off it (clavicles, neck) inherits the full upper-body rotation.
		const TMap<FName, FName> SourceBoneOverrides{
			{TEXT("spine_02"), TEXT("spine_03")},
			{TEXT("spine_03"), TEXT("spine_05")},
		};

		// Epic's IK bones mirror their hand/foot in component space.
		const TMap<FName, FName> IkBoneTargets{
			{TEXT("ik_foot_root"), TEXT("root")},
			{TEXT("ik_hand_root"), TEXT("root")},
			{TEXT("ik_foot_l"), TEXT("foot_l")},
			{TEXT("ik_foot_r"), TEXT("foot_r")},
			{TEXT("ik_hand_l"), TEXT("hand_l")},
			{TEXT("ik_hand_r"), TEXT("hand_r")},
			{TEXT("ik_hand_gun"), TEXT("hand_r")},
		};

		TArray<int32> TargetToSource;
		TargetToSource.SetNum(TargetBoneCount);
		for (int32 TargetIndex{0}; TargetIndex < TargetBoneCount; ++TargetIndex)
		{
			const FName BoneName{TargetRef.GetBoneName(TargetIndex)};
			const FName* Override{SourceBoneOverrides.Find(BoneName)};
			const int32 SourceIndex{SourceRef.FindBoneIndex(Override != nullptr ? *Override : BoneName)};
			TargetToSource[TargetIndex] = SourceIndex < SourceBoneCount ? SourceIndex : INDEX_NONE;
		}
		TargetToSource[0] = 0;

		const int32 SourcePelvis{SourceRef.FindBoneIndex(TEXT("pelvis"))};
		const int32 TargetPelvis{TargetRef.FindBoneIndex(TEXT("pelvis"))};
		const double MotionScale{
			SourcePelvis != INDEX_NONE && TargetPelvis != INDEX_NONE && SourceRestComponent[SourcePelvis].GetLocation().Z > UE_KINDA_SMALL_NUMBER
				? TargetRestComponent[TargetPelvis].GetLocation().Z / SourceRestComponent[SourcePelvis].GetLocation().Z
				: 1.0
		};

		const IAnimationDataModel* SourceModel{Source->GetDataModel()};
		const int32 NumKeys{SourceModel->GetNumberOfKeys()};

		TArray<TArray<FVector3f>> Positions;
		TArray<TArray<FQuat4f>> Rotations;
		Positions.SetNum(TargetBoneCount);
		Rotations.SetNum(TargetBoneCount);

		TArray<FTransform> SourceLocal;
		TArray<FTransform> SourceComponent;
		TArray<FTransform> TargetComponent;
		SourceLocal.SetNum(SourceBoneCount);
		TargetComponent.SetNum(TargetBoneCount);

		double MaxHandError{0.0};
		const int32 SourceHand{SourceRef.FindBoneIndex(TEXT("hand_r"))};
		const int32 TargetHand{TargetRef.FindBoneIndex(TEXT("hand_r"))};

		for (int32 Key{0}; Key < NumKeys; ++Key)
		{
			for (int32 SourceIndex{0}; SourceIndex < SourceBoneCount; ++SourceIndex)
			{
				const FName BoneName{SourceRef.GetBoneName(SourceIndex)};
				SourceLocal[SourceIndex] = SourceModel->IsValidBoneTrackName(BoneName)
					                           ? SourceModel->GetBoneTrackTransform(BoneName, FFrameNumber{Key})
					                           : SourceRestLocal[SourceIndex];
			}
			FAnimationRuntime::FillUpComponentSpaceTransforms(SourceRef, SourceLocal, SourceComponent);

			// Pass 1: everything but IK bones, parents before children.
			for (int32 TargetIndex{0}; TargetIndex < TargetBoneCount; ++TargetIndex)
			{
				if (IkBoneTargets.Contains(TargetRef.GetBoneName(TargetIndex)))
				{
					continue;
				}

				const int32 SourceIndex{TargetToSource[TargetIndex]};
				const int32 ParentIndex{TargetRef.GetParentIndex(TargetIndex)};

				FQuat Rotation;
				if (SourceIndex != INDEX_NONE)
				{
					const FQuat SourceDelta{SourceComponent[SourceIndex].GetRotation() * SourceRestComponent[SourceIndex].GetRotation().Inverse()};
					Rotation = SourceDelta * TargetRestComponent[TargetIndex].GetRotation();
				}
				else
				{
					Rotation = TargetComponent[ParentIndex].GetRotation() * TargetRestLocal[TargetIndex].GetRotation();
				}

				FVector Location;
				if ((TargetIndex == 0 || TargetIndex == TargetPelvis) && SourceIndex != INDEX_NONE)
				{
					const FVector SourceMotion{SourceComponent[SourceIndex].GetLocation() - SourceRestComponent[SourceIndex].GetLocation()};
					Location = TargetRestComponent[TargetIndex].GetLocation() + SourceMotion * MotionScale;
				}
				else
				{
					Location = TargetComponent[ParentIndex].TransformPosition(TargetRestLocal[TargetIndex].GetLocation());
				}

				TargetComponent[TargetIndex] = FTransform{Rotation.GetNormalized(), Location};
			}

			// Pass 2: IK bones copy their hand/foot (or root) in component space.
			for (int32 TargetIndex{0}; TargetIndex < TargetBoneCount; ++TargetIndex)
			{
				if (const FName* Mirror{IkBoneTargets.Find(TargetRef.GetBoneName(TargetIndex))})
				{
					const int32 MirrorIndex{TargetRef.FindBoneIndex(*Mirror)};
					TargetComponent[TargetIndex] = MirrorIndex != INDEX_NONE ? TargetComponent[MirrorIndex] : TargetComponent[0];
				}
			}

			for (int32 TargetIndex{0}; TargetIndex < TargetBoneCount; ++TargetIndex)
			{
				const int32 ParentIndex{TargetRef.GetParentIndex(TargetIndex)};
				const FTransform Local{
					ParentIndex == INDEX_NONE ? TargetComponent[TargetIndex] : TargetComponent[TargetIndex].GetRelativeTransform(TargetComponent[ParentIndex])
				};

				Positions[TargetIndex].Add(FVector3f{Local.GetLocation()});
				Rotations[TargetIndex].Add(FQuat4f{Local.GetRotation().GetNormalized()});
			}

			if (SourceHand != INDEX_NONE && TargetHand != INDEX_NONE)
			{
				MaxHandError = FMath::Max(MaxHandError, FVector::Dist(
					                          SourceComponent[SourceHand].GetLocation() * MotionScale,
					                          TargetComponent[TargetHand].GetLocation()));
			}
		}

		UPackage* Package{PrepareFreshPackage(PackagePath, AssetName)};
		auto* Target{NewObject<UAnimSequence>(Package, *AssetName, RF_Public | RF_Standalone)};
		Target->SetSkeleton(TargetSkeleton);
		Target->SetPreviewMesh(PreviewMesh);

		IAnimationDataController& Controller{Target->GetController()};
		Controller.OpenBracket(FText::FromString(TEXT("BK Retarget")), false);
		Controller.InitializeModel();
		Controller.SetFrameRate(SourceModel->GetFrameRate(), false);
		Controller.SetNumberOfFrames(FFrameNumber{FMath::Max(NumKeys - 1, 1)}, false);

		const TArray<FVector3f> UnitScales{
			[&] { TArray<FVector3f> Scales; Scales.Init(FVector3f::OneVector, NumKeys); return Scales; }()
		};

		for (int32 TargetIndex{0}; TargetIndex < TargetBoneCount; ++TargetIndex)
		{
			const FName BoneName{TargetRef.GetBoneName(TargetIndex)};
			Controller.AddBoneCurve(BoneName, false);
			Controller.SetBoneTrackKeys(BoneName, Positions[TargetIndex], Rotations[TargetIndex], UnitScales, false);
		}

		AddAlsActionCurves(Controller, Target, AlsRoll);

		Controller.NotifyPopulated();
		Controller.CloseBracket(false);

		Target->bEnableRootMotion = Source->bEnableRootMotion;
		Target->RootMotionRootLock = Source->RootMotionRootLock;
		Target->bForceRootLock = Source->bForceRootLock;

		// Differences in rest pose and proportions show up here; a detached hand would be tens of cm.
		UE_LOG(LogTemp, Display, TEXT("  retargeted %s -> %s (%d keys, hip scale %.3f, max hand_r deviation %.1f cm)"),
		       *Source->GetName(), *AssetName, NumKeys, MotionScale, MaxHandError);

		return SaveAsset(Package, Target) ? Target : nullptr;
	}

	/**
	 * Tells ALS to treat this animation as a full-body action instead of layering its overlay
	 * (arms, spine, head) over it, by copying the layering curve values from the middle of ALS's
	 * own roll - where -1 cancels the overlay that the base pose holds at 1.
	 *
	 * These go on the sequence, not the montage: montage-level curves are not evaluated at runtime.
	 * Must be called inside an open controller bracket for Sequence.
	 */
	void AddAlsActionCurves(IAnimationDataController& Controller, const UAnimSequence* Sequence, const UAnimMontage* AlsRoll)
	{
		const float SampleTime{AlsRoll->GetPlayLength() * 0.5f};
		const float SequenceLength{static_cast<float>(Sequence->GetDataModel()->GetPlayLength())};

		for (const FFloatCurve& RollCurve : AlsRoll->GetCurveData().FloatCurves)
		{
			// Foot IK/lock mid-roll is transient (feet in the air); leave feet to ALS's locomotion.
			const FString CurveName{RollCurve.GetName().ToString()};
			if (!CurveName.StartsWith(TEXT("Layer")) && CurveName != TEXT("ViewBlock"))
			{
				continue;
			}

			const float Value{RollCurve.FloatCurve.Eval(SampleTime)};
			const FAnimationCurveIdentifier CurveId{RollCurve.GetName(), ERawCurveTrackTypes::RCT_Float};
			Controller.AddCurve(CurveId, AACF_DefaultCurve, false);
			Controller.SetCurveKeys(CurveId, {FRichCurveKey{0.0f, Value}, FRichCurveKey{SequenceLength, Value}}, false);
		}
	}

	void AddNotify(UAnimMontage* Montage, UAnimNotify* Notify, const float Time)
	{
		FAnimNotifyEvent& Event{Montage->Notifies.AddDefaulted_GetRef()};
		Event.Link(Montage, Time);
		Event.TriggerTimeOffset = GetTriggerTimeOffsetForType(Montage->CalculateOffsetForNotify(Time));
		Event.TrackIndex = 0;
		Event.Notify = Notify;
		Event.NotifyTriggerChance = 1.0f;
	}

	UAnimMontage* BuildStepMontage(const FBKStepSpec& Spec, const FBKClipTiming& Timing, UAnimSequence* Clip,
	                               USkeleton* Skeleton, USkeletalMesh* PreviewMesh)
	{
		const FString PackagePath{FString::Printf(TEXT("%s/%s"), OutputFolder, Spec.MontageName)};
		UPackage* Package{PrepareFreshPackage(PackagePath, Spec.MontageName)};

		auto* Montage{NewObject<UAnimMontage>(Package, Spec.MontageName, RF_Public | RF_Standalone)};
		Montage->SetSkeleton(Skeleton);
		Montage->SetPreviewMesh(PreviewMesh);

		const float ClipStart{Timing.ClipStart};
		const float ClipEnd{Timing.ClipEnd > ClipStart ? Timing.ClipEnd : Clip->GetPlayLength()};

		FSlotAnimationTrack& Track{Montage->SlotAnimTracks[0]};
		Track.SlotName = AlsFullBodySlot;

		FAnimSegment Segment;
		Segment.SetAnimReference(Clip, true);
		Segment.AnimStartTime = ClipStart;
		Segment.AnimEndTime = ClipEnd;
		Segment.AnimPlayRate = Spec.PlayRate;
		Segment.StartPos = 0.0f;
		Track.AnimTrack.AnimSegments.Add(Segment);

		const float Length{Segment.GetLength()};
		Montage->SetCompositeLength(Length);

		FCompositeSection& Section{Montage->CompositeSections.AddDefaulted_GetRef()};
		Section.SectionName = TEXT("Default");
		Section.Link(Montage, 0.0f);

		// Cubic in/out: combo steps cross-fade into each other, and the last ~0.35s of a
		// swing fades into locomotion instead of cutting back to idle.
		Montage->BlendIn.SetBlendTime(0.15f);
		Montage->BlendIn.SetBlendOption(EAlphaBlendOption::HermiteCubic);
		Montage->BlendOut.SetBlendTime(0.35f);
		Montage->BlendOut.SetBlendOption(EAlphaBlendOption::HermiteCubic);
		Montage->BlendOutTriggerTime = -1.0f;
		Montage->bEnableAutoBlendOut = true;

		Montage->AnimNotifyTracks.Emplace(TEXT("1"), FLinearColor::White);

		const auto ToMontageTime{[&](const float ClipTime) { return FMath::Clamp((ClipTime - ClipStart) / Spec.PlayRate, 0.0f, Length); }};

		float LastTraceTime{ClipStart};
		for (const TPair<float, FName>& Trace : Timing.Traces)
		{
			auto* Notify{NewObject<UBKAnimNotify_AttackTrace>(Montage)};
			Notify->SourceBone = Trace.Value;
			AddNotify(Montage, Notify, ToMontageTime(Trace.Key));
			LastTraceTime = FMath::Max(LastTraceTime, Trace.Key);

			UE_LOG(LogTemp, Display, TEXT("  %s: trace (%s) @ %.3fs"), Spec.MontageName, *Trace.Value.ToString(), ToMontageTime(Trace.Key));
		}

		if (Spec.bComboWindow)
		{
			const float WindowClipTime{Timing.ComboWindow.Get(LastTraceTime + DefaultComboWindowDelay)};
			AddNotify(Montage, NewObject<UBKAnimNotify_ComboWindow>(Montage), ToMontageTime(WindowClipTime));

			UE_LOG(LogTemp, Display, TEXT("  %s: combo window @ %.3fs%s"), Spec.MontageName, ToMontageTime(WindowClipTime),
			       Timing.ComboWindow.IsSet() ? TEXT("") : TEXT(" (derived)"));
		}

		Montage->RefreshCacheData();

		UE_LOG(LogTemp, Display, TEXT("  %s: %s @ %.2fx, %.3fs"), Spec.MontageName, *Clip->GetName(), Spec.PlayRate, Length);

		return SaveAsset(Package, Montage) ? Montage : nullptr;
	}

	/** Placement of the guard, in character space (cm): X forward, Y out toward that arm's side, Z up. */
	struct FBKGuardTuning
	{
		/** Fist position relative to the head bone. */
		FVector FistFromHead{24.0, 10.0, -16.0};

		/** Elbow hint relative to the upper arm: keeps elbows tucked down and slightly out. */
		FVector ElbowFromShoulder{12.0, 18.0, -50.0};

		/** Degrees each finger joint curls to form a fist. */
		double FingerCurlDegrees{75.0};
	};

	/**
	 * Builds a held high-guard pose on ALS's skeleton: ALS's standing pose, with each arm solved by
	 * two-bone IK so the fists sit in front of the face and the elbows stay low, and fingers curled.
	 */
	UAnimSequence* BakeGuardPose(USkeleton* Skeleton, USkeletalMesh* PreviewMesh, const FBKGuardTuning& Tuning, const UAnimMontage* AlsRoll)
	{
		const auto* StandPose{LoadObject<UAnimSequence>(nullptr, AlsStandPosePath)};
		if (!IsValid(StandPose))
		{
			UE_LOG(LogTemp, Error, TEXT("BKCreateCombatAnimations: failed to load %s."), AlsStandPosePath);
			return nullptr;
		}

		const FReferenceSkeleton& Ref{Skeleton->GetReferenceSkeleton()};
		const int32 BoneCount{Ref.GetRawBoneNum()};
		const TArrayView<const FTransform> RestLocal{Ref.GetRawRefBonePose()};
		const IAnimationDataModel* StandModel{StandPose->GetDataModel()};

		TArray<FTransform> Local;
		Local.SetNum(BoneCount);
		for (int32 Index{0}; Index < BoneCount; ++Index)
		{
			const FName BoneName{Ref.GetBoneName(Index)};
			Local[Index] = StandModel->IsValidBoneTrackName(BoneName) ? StandModel->GetBoneTrackTransform(BoneName, FFrameNumber{0}) : RestLocal[Index];
		}

		TArray<FTransform> Component;
		FAnimationRuntime::FillUpComponentSpaceTransforms(Ref, Local, Component);

		const auto Find{[&Ref](const FString& Name) { return Ref.FindBoneIndex(*Name); }};
		const int32 Head{Find(TEXT("head"))};
		const int32 FootLeft{Find(TEXT("foot_l"))};
		const int32 FootRight{Find(TEXT("foot_r"))};
		const int32 BallLeft{Find(TEXT("ball_l"))};
		const int32 BallRight{Find(TEXT("ball_r"))};
		if (Head == INDEX_NONE || FootLeft == INDEX_NONE || FootRight == INDEX_NONE || BallLeft == INDEX_NONE || BallRight == INDEX_NONE)
		{
			UE_LOG(LogTemp, Error, TEXT("BKCreateCombatAnimations: SK_Als is missing head/foot/ball bones."));
			return nullptr;
		}

		// Derive the character's facing from the skeleton itself (toes are in front of ankles)
		// rather than assuming an axis convention.
		FVector Forward{Component[BallLeft].GetLocation() + Component[BallRight].GetLocation() -
		                Component[FootLeft].GetLocation() - Component[FootRight].GetLocation()};
		Forward.Z = 0.0;
		Forward.Normalize();

		for (const TCHAR* Side : {TEXT("l"), TEXT("r")})
		{
			const int32 Clavicle{Find(FString::Printf(TEXT("clavicle_%s"), Side))};
			const int32 OtherClavicle{Find(FString::Printf(TEXT("clavicle_%s"), FCString::Strcmp(Side, TEXT("l")) == 0 ? TEXT("r") : TEXT("l")))};
			const int32 UpperArm{Find(FString::Printf(TEXT("upperarm_%s"), Side))};
			const int32 LowerArm{Find(FString::Printf(TEXT("lowerarm_%s"), Side))};
			const int32 Hand{Find(FString::Printf(TEXT("hand_%s"), Side))};
			if (Clavicle == INDEX_NONE || OtherClavicle == INDEX_NONE || UpperArm == INDEX_NONE || LowerArm == INDEX_NONE || Hand == INDEX_NONE)
			{
				UE_LOG(LogTemp, Error, TEXT("BKCreateCombatAnimations: SK_Als is missing %s arm bones."), Side);
				return nullptr;
			}

			FVector Outward{Component[Clavicle].GetLocation() - Component[OtherClavicle].GetLocation()};
			Outward -= Forward * FVector::DotProduct(Outward, Forward);
			Outward.Z = 0.0;
			Outward.Normalize();

			const auto ToComponent{[&](const FVector& Offset) { return Forward * Offset.X + Outward * Offset.Y + FVector::UpVector * Offset.Z; }};
			const FVector Fist{Component[Head].GetLocation() + ToComponent(Tuning.FistFromHead)};
			const FVector ElbowHint{Component[UpperArm].GetLocation() + ToComponent(Tuning.ElbowFromShoulder)};

			FTransform UpperArmTransform{Component[UpperArm]};
			FTransform LowerArmTransform{Component[LowerArm]};
			FTransform HandTransform{Component[Hand]};
			const FQuat ForearmToHand{Component[LowerArm].GetRotation().Inverse() * Component[Hand].GetRotation()};

			AnimationCore::SolveTwoBoneIK(UpperArmTransform, LowerArmTransform, HandTransform, ElbowHint, Fist, false, 1.0, 1.0);

			// Straight wrist: keep the hand's rest offset from the (now re-aimed) forearm.
			HandTransform.SetRotation(LowerArmTransform.GetRotation() * ForearmToHand);

			Local[UpperArm] = UpperArmTransform.GetRelativeTransform(Component[Clavicle]);
			Local[LowerArm] = LowerArmTransform.GetRelativeTransform(UpperArmTransform);
			Local[Hand] = HandTransform.GetRelativeTransform(LowerArmTransform);

			UE_LOG(LogTemp, Display, TEXT("  guard %s: fist target %s, reached %s"), Side, *Fist.ToCompactString(),
			       *HandTransform.GetLocation().ToCompactString());
		}

		// Curl fingers into fists (thumbs left alone). UE mannequin-layout finger joints bend about local Z.
		for (int32 Index{0}; Index < BoneCount; ++Index)
		{
			const FString BoneName{Ref.GetBoneName(Index).ToString()};
			if (BoneName.StartsWith(TEXT("index_")) || BoneName.StartsWith(TEXT("middle_")) ||
			    BoneName.StartsWith(TEXT("ring_")) || BoneName.StartsWith(TEXT("pinky_")))
			{
				const FQuat Curl{FVector::ZAxisVector, FMath::DegreesToRadians(-Tuning.FingerCurlDegrees)};
				Local[Index].SetRotation(Local[Index].GetRotation() * Curl);
			}
		}

		const FString AssetName{TEXT("A_BK_GuardPose")};
		const FString PackagePath{FString::Printf(TEXT("%s/Retargeted/%s"), OutputFolder, *AssetName)};
		UPackage* Package{PrepareFreshPackage(PackagePath, AssetName)};

		auto* Pose{NewObject<UAnimSequence>(Package, *AssetName, RF_Public | RF_Standalone)};
		Pose->SetSkeleton(Skeleton);
		Pose->SetPreviewMesh(PreviewMesh);

		// Half a second of held pose; the block montage loops it for as long as Block is held.
		constexpr int32 NumKeys{16};
		IAnimationDataController& Controller{Pose->GetController()};
		Controller.OpenBracket(FText::FromString(TEXT("BK Guard Pose")), false);
		Controller.InitializeModel();
		Controller.SetFrameRate(FFrameRate{30, 1}, false);
		Controller.SetNumberOfFrames(FFrameNumber{NumKeys - 1}, false);

		for (int32 Index{0}; Index < BoneCount; ++Index)
		{
			const FName BoneName{Ref.GetBoneName(Index)};
			TArray<FVector3f> Positions;
			TArray<FQuat4f> Rotations;
			TArray<FVector3f> Scales;
			Positions.Init(FVector3f{Local[Index].GetLocation()}, NumKeys);
			Rotations.Init(FQuat4f{Local[Index].GetRotation().GetNormalized()}, NumKeys);
			Scales.Init(FVector3f::OneVector, NumKeys);

			Controller.AddBoneCurve(BoneName, false);
			Controller.SetBoneTrackKeys(BoneName, Positions, Rotations, Scales, false);
		}

		AddAlsActionCurves(Controller, Pose, AlsRoll);

		Controller.NotifyPopulated();
		Controller.CloseBracket(false);

		return SaveAsset(Package, Pose) ? Pose : nullptr;
	}

	/**
	 * Looping montage that holds the guard pose while Block is held.
	 *
	 * Full-body (PostLocomotion), like the attacks: ALS's per-body-part layering slots (ArmLeft/
	 * ArmRight) would keep the legs walking, but driving them from a montage did not blend through,
	 * even with their Layer*Slot curves set. Full-body means blocking plants the character, so
	 * ABKPlayerCharacter refuses movement input while blocking.
	 */
	UAnimMontage* BuildBlockMontage(UAnimSequence* GuardPose, USkeleton* Skeleton, USkeletalMesh* PreviewMesh)
	{
		const FString AssetName{TEXT("AM_BK_Block")};
		UPackage* Package{PrepareFreshPackage(FString::Printf(TEXT("%s/%s"), OutputFolder, *AssetName), AssetName)};

		auto* Montage{NewObject<UAnimMontage>(Package, *AssetName, RF_Public | RF_Standalone)};
		Montage->SetSkeleton(Skeleton);
		Montage->SetPreviewMesh(PreviewMesh);

		FAnimSegment Segment;
		Segment.SetAnimReference(GuardPose, true);

		// Loop the held pose for ten minutes; Montage_Stop ends it when Block is released.
		Segment.LoopingCount = FMath::CeilToInt32(600.0f / FMath::Max(GuardPose->GetPlayLength(), UE_KINDA_SMALL_NUMBER));

		Montage->SlotAnimTracks[0].SlotName = AlsFullBodySlot;
		Montage->SlotAnimTracks[0].AnimTrack.AnimSegments.Add(Segment);

		Montage->SetCompositeLength(Segment.GetLength());

		// The single section loops into itself, so the guard holds until Montage_Stop.
		FCompositeSection& Section{Montage->CompositeSections.AddDefaulted_GetRef()};
		Section.SectionName = TEXT("Default");
		Section.NextSectionName = TEXT("Default");
		Section.Link(Montage, 0.0f);

		Montage->BlendIn.SetBlendTime(0.12f);
		Montage->BlendIn.SetBlendOption(EAlphaBlendOption::HermiteCubic);
		Montage->BlendOut.SetBlendTime(0.2f);
		Montage->BlendOut.SetBlendOption(EAlphaBlendOption::HermiteCubic);
		Montage->bEnableAutoBlendOut = false;

		Montage->RefreshCacheData();

		return SaveAsset(Package, Montage) ? Montage : nullptr;
	}
}

int32 UBKCreateCombatAnimationsCommandlet::Main(const FString& Params)
{
	auto* AlsSkeleton{LoadObject<USkeleton>(nullptr, AlsSkeletonPath)};
	auto* AlsMesh{LoadObject<USkeletalMesh>(nullptr, AlsMeshPath)};
	const auto* AlsRoll{LoadObject<UAnimMontage>(nullptr, AlsRollMontagePath)};
	if (!IsValid(AlsSkeleton) || !IsValid(AlsMesh) || !IsValid(AlsRoll))
	{
		UE_LOG(LogTemp, Error, TEXT("BKCreateCombatAnimations: failed to load ALS skeleton, mesh or roll montage."));
		return 1;
	}

	for (const FAnimSlotGroup& Group : AlsSkeleton->GetSlotGroups())
	{
		TArray<FString> SlotNames;
		Algo::Transform(Group.SlotNames, SlotNames, [](const FName& Name) { return Name.ToString(); });
		UE_LOG(LogTemp, Display, TEXT("BKCreateCombatAnimations: SK_Als slot group '%s': %s"),
		       *Group.GroupName.ToString(), *FString::Join(SlotNames, TEXT(", ")));
	}

	TMap<FName, FBKClipTiming> Timings;
	GatherTemplateTimings(Timings);

	TMap<FName, TObjectPtr<UAnimSequence>> RetargetedClips;

	for (const FBKStepSpec& Spec : StepSpecs)
	{
		const FBKClipTiming* Timing{Timings.Find(Spec.SourceClip)};
		if (Timing == nullptr || Timing->Traces.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("BKCreateCombatAnimations: no template timing (or no hit trace) for %s."), Spec.SourceClip);
			return 1;
		}

		TObjectPtr<UAnimSequence>& Clip{RetargetedClips.FindOrAdd(Spec.SourceClip)};
		if (!IsValid(Clip))
		{
			Clip = BakeRetargetedClip(Timing->Sequence, AlsSkeleton, AlsMesh, AlsRoll);
			if (!IsValid(Clip))
			{
				return 1;
			}
		}

		if (!IsValid(BuildStepMontage(Spec, *Timing, Clip, AlsSkeleton, AlsMesh)))
		{
			return 1;
		}
	}

	auto* GuardPose{BakeGuardPose(AlsSkeleton, AlsMesh, FBKGuardTuning{}, AlsRoll)};
	if (!IsValid(GuardPose) || !IsValid(BuildBlockMontage(GuardPose, AlsSkeleton, AlsMesh)))
	{
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("BKCreateCombatAnimations: built %d combo step montages and the block montage."), UE_ARRAY_COUNT(StepSpecs));
	return 0;
}
#else
int32 UBKCreateCombatAnimationsCommandlet::Main(const FString& Params)
{
	UE_LOG(LogTemp, Error, TEXT("BKCreateCombatAnimations: editor-only commandlet, WITH_EDITOR=0."));
	return 1;
}
#endif
