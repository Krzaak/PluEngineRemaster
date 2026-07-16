//
// Created by Plutex on 7/6/26.
//

#include "PluEngine/BasicEngineClasses/Components/SkeletalMeshComponent.h"

#include <cmath>

#include "PluEngine/PluUtils.h"
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
	mNodesCache.Clear();
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

bool Plu::SkeletalMeshComponent::TryGetAttachPointWorldMatrix(const String& attachPointName, Matrix4& outMatrix)
{
	if (!SkeletalMeshToDisplay || !SkeletalMeshToDisplay->MeshSkeleton) {
		return false;
	}
	TOwningPointer<SkeletonAttachPoint>* attachPointFind = SkeletalMeshToDisplay->MeshSkeleton->AttachPoints.Find(attachPointName);
	if (attachPointFind == nullptr) {
		PLU_CORE_ERROR("No attach point named {0}", attachPointName.CStr());
		return false;
	}
	TUsePointer<SkeletonAttachPoint> attachPoint = *attachPointFind;
	TUsePointer<SkeletonNode> parentNode;
	if (mNodesCache.Contains(attachPoint->ParentNodeName)) {
		parentNode = mNodesCache[attachPoint->ParentNodeName];
	} else {
		for (auto node : mNodes) {
			if (node->NodeName == attachPoint->ParentNodeName) {
				parentNode = node;
				mNodesCache.Insert(attachPoint->ParentNodeName, parentNode);
				break;
			}
		}
	}
	if (!parentNode) {
		PLU_CORE_ERROR("No Node with name {0}", attachPoint->ParentNodeName.CStr());
		return false;
	}
	if (!mLastBoneGlobalTransforms.Contains(parentNode->NodeName)) {
		return false;
	}

	// mLastBoneGlobalTransforms is skeleton-space (root-relative), so the component's own world
	// matrix has to go in front of it, and the attach point's offset has to ride the posed bone's
	// frame instead of being added along world axes.
	outMatrix = GetWorldMatrix() * mLastBoneGlobalTransforms[parentNode->NodeName] * attachPoint->GetLocalMatrix();
	return true;
}

Vec3 Plu::SkeletalMeshComponent::GetAttachPointLocationInWorld(String attachPointName)
{
	Matrix4 attachPointWorld;
	if (!TryGetAttachPointWorldMatrix(attachPointName, attachPointWorld)) {
		return this->GetWorldLocation();
	}
	return GetLocationFromMatrix(attachPointWorld);
}

Vec3 Plu::SkeletalMeshComponent::GetAttachPointRotationInWorld(String attachPointName)
{
	Matrix4 attachPointWorld;
	if (!TryGetAttachPointWorldMatrix(attachPointName, attachPointWorld)) {
		return this->GetWorldRotation();
	}
	return GetRotationFromMatrix(attachPointWorld);
}

void Plu::SkeletalMeshComponent::InvalidateGlobalTransforms()
{
	mLastBoneGlobalTransforms.Clear();
}

void Plu::SkeletalMeshComponent::InsertGlobalTransform(const String &node, const Matrix4 &globalTransform)
{
	mLastBoneGlobalTransforms[node] = globalTransform;
}
