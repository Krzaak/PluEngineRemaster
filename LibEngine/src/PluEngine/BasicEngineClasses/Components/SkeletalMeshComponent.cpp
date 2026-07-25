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
	if (!IsPlaying)
	{
		mWasPlaying = false;
		return;
	}

	// Graph time runs independently of the raw-animation tick head below — the graph has no
	// single FPS of its own, and a component may have both assigned (graph wins at render time,
	// see RenderSnapshotBuilder) or just one.
	if (AnimGraph)
	{
		GraphTimeSeconds += deltaTime;
		if (AnimGraphInstance* instance = EnsureAnimGraphInstance()) {
			instance->TimeSeconds = GraphTimeSeconds;
		}
	}

	if (!AnimationToShow)
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
	mAttachPointNodeCache.Clear();
	PosedGlobalTransforms.Clear();
	CachedPoseValid = false;
}

Plu::TUsePointer<Plu::MaterialInfo> Plu::SkeletalMeshComponent::GetMaterial()
{
	return Material;
}

void Plu::SkeletalMeshComponent::SetMaterial(TUsePointer<MaterialInfo> material)
{
	Material = material;
}

Plu::TUsePointer<Plu::AnimationGraph> Plu::SkeletalMeshComponent::GetAnimGraph()
{
	return AnimGraph;
}

void Plu::SkeletalMeshComponent::SetAnimGraph(TUsePointer<AnimationGraph> animGraph)
{
	AnimGraph = animGraph;
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

	// Resolve the parent node to a pose-layout index once per attach point, then reuse it.
	Int32 parentIndex;
	if (const Int32* cached = mAttachPointNodeCache.Find(attachPointName)) {
		parentIndex = *cached;
	} else {
		parentIndex = SkeletalMeshToDisplay->MeshSkeleton->GetPoseLayout().FindIndex(attachPoint->ParentNodeName);
		if (parentIndex < 0) {
			PLU_CORE_ERROR("No Node with name {0}", attachPoint->ParentNodeName.CStr());
			return false;
		}
		mAttachPointNodeCache.Insert(attachPointName, parentIndex);
	}

	// Out of range means the pose has not been evaluated yet (no snapshot built since the mesh
	// was assigned), so there is no posed transform to hand out.
	if (static_cast<UInt64>(parentIndex) >= PosedGlobalTransforms.Size()) {
		return false;
	}

	// PosedGlobalTransforms is skeleton-space (root-relative), so the component's own world
	// matrix has to go in front of it, and the attach point's offset has to ride the posed bone's
	// frame instead of being added along world axes.
	outMatrix = GetWorldMatrix()
		* PosedGlobalTransforms[static_cast<UInt64>(parentIndex)].ToMatrix()
		* attachPoint->GetLocalMatrix();
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

Plu::AnimGraphInstance* Plu::SkeletalMeshComponent::EnsureAnimGraphInstance()
{
	if (!AnimGraph) return nullptr;
	if (!mAnimGraphInstance) {
		mAnimGraphInstance = CreateOwning<AnimGraphInstance>();
#ifdef PLU_ENGINE_EDITOR_BUILD
		TUsePointer<GameObject> owner = GetParentGameObject();
		mAnimGraphInstance->DebugName = (owner ? owner->GetObjectName() : String("?"))
			+ " / " + GetClass()->TypeName;
#endif
	}
	mAnimGraphInstance->BindTo(AnimGraph);
	return mAnimGraphInstance.GetRaw();
}

Plu::AnimGraphInstance* Plu::SkeletalMeshComponent::GetAnimGraphInstance()
{
	return EnsureAnimGraphInstance();
}

