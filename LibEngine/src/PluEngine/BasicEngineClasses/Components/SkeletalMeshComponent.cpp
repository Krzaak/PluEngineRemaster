//
// Created by Plutex on 7/6/26.
//

#include "PluEngine/BasicEngineClasses/Components/SkeletalMeshComponent.h"

#include <cmath>

#include "PluEngine/AssetTypes/Animation/SkeletalAnimation.h"
#include "PluEngine/AssetTypes/Skeleton/Skeleton.h"
#include "PluEngine/GameObject/GameObject.h"

void Plu::SkeletalMeshComponent::OnUpdate(float deltaTime)
{
	if (!IsPlaying || !AnimationToShow)
	{
		mWasPlaying = false;
		return;
	}

	if (!mWasPlaying)
	{
		// Play just started (or resumed) — pick up from the scrubbed frame.
		AnimationTimeTicks = static_cast<float>(AnimationFrameToShow);
		mWasPlaying = true;
	}

	AnimationTimeTicks += deltaTime * AnimationToShow->FramesPerSecond;

	const float duration = static_cast<float>(AnimationToShow->FramesAmount);
	if (AnimationTimeTicks > duration)
	{
		if (LoopAnimation && duration > 0.0f)
		{
			AnimationTimeTicks = std::fmod(AnimationTimeTicks, duration);
		}
		else
		{
			AnimationTimeTicks = duration;
			IsPlaying = false;
		}
	}

	// Mirror into the scrub property so pausing freezes on the current pose.
	AnimationFrameToShow = static_cast<int>(AnimationTimeTicks);
}

Plu::TUsePointer<Plu::SkeletalMesh> Plu::SkeletalMeshComponent::GetSkeletalMesh()
{
	return SkeletalMeshToDisplay;
}

void Plu::SkeletalMeshComponent::SetSkeletalMesh(TUsePointer<SkeletalMesh> skeletalMesh)
{
	mNodes.Clear();
	SkeletalMeshToDisplay = skeletalMesh;
	if (SkeletalMeshToDisplay) {
		SkeletalMeshToDisplay->MeshSkeleton->CreateNodePalette(&mNodes);
	}
}

Plu::TUsePointer<Plu::MaterialInfo> Plu::SkeletalMeshComponent::GetMaterial()
{
	return Material;
}

void Plu::SkeletalMeshComponent::SetMaterial(TUsePointer<MaterialInfo> material)
{
	Material = material;
}

Matrix4 Plu::SkeletalMeshComponent::GetRenderMatrix()
{
	return GetWorldMatrix();
}

DynamicArray<Plu::TOwningPointer<Plu::SkeletonNode>> * Plu::SkeletalMeshComponent::GetNodes()
{
	return &mNodes;
}