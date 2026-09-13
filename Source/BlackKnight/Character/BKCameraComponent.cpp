// The Black Knight: Beginnings

#include "BKCameraComponent.h"

#include "AlsCameraSettings.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/ConstructorHelpers.h"

UBKCameraComponent::UBKCameraComponent()
{
	// The ALS camera is a skeletal mesh component: it reads camera lag, pivot and
	// FOV values from curves on its own animation instance, so all three of
	// mesh, animation class and settings must be present or BeginPlay will ensure.

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CameraMesh{
		TEXT("/ALS/ALSCamera/SKM_Als_Camera.SKM_Als_Camera")
	};

	static ConstructorHelpers::FClassFinder<UAnimInstance> CameraAnimation{
		TEXT("/ALS/ALSCamera/AB_Als_Camera")
	};

	static ConstructorHelpers::FObjectFinder<UAlsCameraSettings> CameraSettings{
		TEXT("/ALS/ALSCamera/Data/CS_Als_Default.CS_Als_Default")
	};

	if (CameraMesh.Succeeded())
	{
		SetSkeletalMeshAsset(CameraMesh.Object);
	}

	if (CameraAnimation.Succeeded())
	{
		SetAnimInstanceClass(CameraAnimation.Class);
	}

	if (CameraSettings.Succeeded())
	{
		Settings = CameraSettings.Object;
	}
}
