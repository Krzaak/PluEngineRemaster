//
// Created by Plutex on 7/25/26.
//

#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimTransformBoneNode.h"

namespace Plu
{
	String AnimTransformBoneNode::GetDisplayName()
	{
		return Bone.Name.IsEmpty() ? "Transform Bone" : "Transform Bone (" + Bone.Name + ")";
	}

	Pose AnimTransformBoneNode::Evaluate(AnimEvalContext& context)
	{
		PLU_PROFILE_SCOPE("AnimTransformBone");

		Pose pose = EvaluateInputPose(context, "Pose");

		const bool allIgnored = TranslationMode == EBoneModifyMode::Ignore
			&& RotationMode == EBoneModifyMode::Ignore
			&& ScaleMode == EBoneModifyMode::Ignore;
		const float alpha = glm::clamp(ReadDataPin<float>(context, "Alpha", Alpha), 0.0f, 1.0f);

		if (allIgnored || alpha <= 0.0f || !context.TargetSkeleton) {
			return pose;
		}

		const SkeletonPoseLayout& layout = context.TargetSkeleton->GetPoseLayout();
		const Int32 idx = layout.FindIndex(Bone.Name);
		if (idx < 0) {
			return pose;
		}
		Int32 refIdx = -1;
		if (Space == EBoneTransformSpace::Bone) {
			refIdx = layout.FindIndex(ReferenceBone.Name);
			if (refIdx < 0) {
				return pose; // no reference bone — the authored values have no defined frame
			}
		}

		const Vec3 translation = ReadDataPin<Vec3>(context, "Translation", Translation);
		const Vec3 rotation = ReadDataPin<Vec3>(context, "Rotation", Rotation);
		const Vec3 scale = ReadDataPin<Vec3>(context, "Scale", Scale);
		const Quaternion delta(glm::radians(rotation));

		// Pose out of the pin is local (parent-space); go up to whatever space this node authors in.
		Pose globals;
		if (Space != EBoneTransformSpace::Local) {
			layout.ComposeGlobals(pose, globals);
		}

		BoneTransform target = (Space == EBoneTransformSpace::Local) ? pose[static_cast<UInt64>(idx)]
			: globals[static_cast<UInt64>(idx)];

		if (Space == EBoneTransformSpace::World) {
			target = BoneTransform::FromMatrix(context.ComponentToWorld).Compose(target);
		} else if (Space == EBoneTransformSpace::Bone) {
			target = globals[static_cast<UInt64>(refIdx)].Inverse().Compose(target);
		}

		switch (TranslationMode) {
			case EBoneModifyMode::Add:     target.Location += translation; break;
			case EBoneModifyMode::Replace: target.Location = translation; break;
			case EBoneModifyMode::Ignore:  break;
		}
		switch (RotationMode) {
			case EBoneModifyMode::Add:     target.Rotation = delta * target.Rotation; break;
			case EBoneModifyMode::Replace: target.Rotation = delta; break;
			case EBoneModifyMode::Ignore:  break;
		}
		switch (ScaleMode) {
			case EBoneModifyMode::Add:     target.Scale *= scale; break;
			case EBoneModifyMode::Replace: target.Scale = scale; break;
			case EBoneModifyMode::Ignore:  break;
		}

		// Back down to local — the pose leaving this node is parent-space, same contract as
		// everything else in the graph (ComposeGlobals runs once, downstream, in RenderSnapshotBuilder).
		BoneTransform newLocal;
		if (Space == EBoneTransformSpace::World) {
			const BoneTransform worldXform = BoneTransform::FromMatrix(context.ComponentToWorld);
			target = worldXform.Inverse().Compose(target);
		} else if (Space == EBoneTransformSpace::Bone) {
			target = globals[static_cast<UInt64>(refIdx)].Compose(target);
		}
		if (Space != EBoneTransformSpace::Local) {
			const Int32 parent = layout.ParentIndex[static_cast<UInt64>(idx)];
			const BoneTransform parentGlobal = (parent < 0) ? BoneTransform() : globals[static_cast<UInt64>(parent)];
			newLocal = parentGlobal.Inverse().Compose(target);
		} else {
			newLocal = target;
		}

		pose[static_cast<UInt64>(idx)] = BlendTransforms(pose[static_cast<UInt64>(idx)], newLocal, alpha);
		pose[static_cast<UInt64>(idx)].NormalizeRotation();
		return pose;
	}
}
