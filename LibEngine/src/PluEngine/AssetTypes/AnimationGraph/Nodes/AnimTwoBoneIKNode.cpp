//
// Created by Plutex on 7/26/26.
//

#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimTwoBoneIKNode.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>

namespace Plu
{
	namespace
	{
		// Shortest-arc rotation taking direction `from` to direction `to` (any non-zero lengths).
		Quaternion RotationBetween(const Vec3& from, const Vec3& to)
		{
			const Vec3 f = glm::normalize(from);
			const Vec3 t = glm::normalize(to);
			const float cosAngle = glm::clamp(glm::dot(f, t), -1.0f, 1.0f);
			if (cosAngle > 1.0f - 1e-6f) {
				return Quaternion(1.0f, 0.0f, 0.0f, 0.0f);
			}
			if (cosAngle < -1.0f + 1e-6f) {
				// Opposite directions: 180° around any axis perpendicular to `f`.
				Vec3 axis = glm::cross(Vec3(1.0f, 0.0f, 0.0f), f);
				if (glm::dot(axis, axis) < 1e-8f) {
					axis = glm::cross(Vec3(0.0f, 1.0f, 0.0f), f);
				}
				return glm::angleAxis(glm::pi<float>(), glm::normalize(axis));
			}
			return glm::angleAxis(glm::acos(cosAngle), glm::normalize(glm::cross(f, t)));
		}
	}

	String AnimTwoBoneIKNode::GetDisplayName()
	{
		return Bone.Name.IsEmpty() ? "Two Bone IK" : "Two Bone IK (" + Bone.Name + ")";
	}

	Pose AnimTwoBoneIKNode::Evaluate(AnimEvalContext& context)
	{
		PLU_PROFILE_SCOPE("AnimTwoBoneIK");

		Pose pose = EvaluateInputPose(context, "Pose");

		const float alpha = glm::clamp(ReadDataPin<float>(context, "Alpha", Alpha), 0.0f, 1.0f);
		if (alpha <= 0.0f || !context.TargetSkeleton) {
			return pose;
		}

		const SkeletonPoseLayout& layout = context.TargetSkeleton->GetPoseLayout();
		const Int32 tipSigned = layout.FindIndex(Bone.Name);
		if (tipSigned < 0) {
			return pose;
		}
		const Int32 midSigned = layout.ParentIndex[static_cast<UInt64>(tipSigned)];
		if (midSigned < 0) {
			return pose; // tip is the root — no chain to solve
		}
		const Int32 rootSigned = layout.ParentIndex[static_cast<UInt64>(midSigned)];
		if (rootSigned < 0) {
			return pose; // chain shorter than three nodes
		}
		const UInt64 tip = static_cast<UInt64>(tipSigned);
		const UInt64 mid = static_cast<UInt64>(midSigned);
		const UInt64 root = static_cast<UInt64>(rootSigned);

		// The whole solve happens in component space; ComposeGlobals gets us there from the local pose.
		Pose globals;
		layout.ComposeGlobals(pose, globals);

		const Vec3 rootPos = globals[root].Location;
		const Vec3 midPos = globals[mid].Location;
		const Vec3 tipPos = globals[tip].Location;

		const float upperLen = glm::length(midPos - rootPos);
		const float lowerLen = glm::length(tipPos - midPos);
		if (upperLen < 1e-5f || lowerLen < 1e-5f) {
			return pose; // zero-length bone — the solve is undefined
		}

		// Goal into component space (the space `globals` lives in).
		Vec3 effector = ReadDataPin<Vec3>(context, "EffectorLocation", EffectorLocation);
		if (EffectorSpace == EIKGoalSpace::World) {
			effector = BoneTransform::FromMatrix(context.ComponentToWorld).Inverse().TransformPoint(effector);
		}

		// Chain direction root→effector; effector sitting on the root degenerates to the current
		// chain direction so the solve stays stable instead of snapping.
		const Vec3 toEffector = effector - rootPos;
		const float toEffectorLen = glm::length(toEffector);
		const Vec3 dir = (toEffectorLen > 1e-5f) ? toEffector / toEffectorLen
			: glm::normalize(midPos - rootPos);

		// Optionally stretch both bones (uniformly) toward an out-of-reach effector.
		float upper = upperLen;
		float lower = lowerLen;
		const float chainLen = upperLen + lowerLen;
		if (toEffectorLen > chainLen && ReadDataPin<bool>(context, "AllowStretch", AllowStretch)) {
			const float maxRatio = glm::max(ReadDataPin<float>(context, "MaxStretchRatio", MaxStretchRatio), 1.0f);
			const float stretch = glm::min(toEffectorLen / chainLen, maxRatio);
			upper *= stretch;
			lower *= stretch;
		}

		// Reach clamped between fully folded and fully straight (epsilons keep the triangle
		// non-degenerate so the law of cosines below stays well-conditioned).
		const float reach = glm::clamp(toEffectorLen,
			glm::abs(upper - lower) + 1e-4f, upper + lower - 1e-4f);

		// Bend direction: the pole target's component perpendicular to the chain axis. With
		// AutoJointTarget there is no pole — the bend plane comes straight from the incoming pose
		// (where the animation already put the joint). A pole on the axis degenerates to the same
		// pose-derived plane; a straight incoming chain falls back to an arbitrary perpendicular.
		Vec3 bendDir(0.0f);
		if (!ReadDataPin<bool>(context, "AutoJointTarget", AutoJointTarget)) {
			Vec3 pole = ReadDataPin<Vec3>(context, "JointTargetLocation", JointTargetLocation);
			if (JointTargetSpace == EIKGoalSpace::World) {
				pole = BoneTransform::FromMatrix(context.ComponentToWorld).Inverse().TransformPoint(pole);
			}
			const Vec3 toPole = pole - rootPos;
			bendDir = toPole - dir * glm::dot(toPole, dir);
		}
		if (glm::dot(bendDir, bendDir) < 1e-8f) {
			const Vec3 toMid = midPos - rootPos;
			bendDir = toMid - dir * glm::dot(toMid, dir);
		}
		if (glm::dot(bendDir, bendDir) < 1e-8f) {
			bendDir = glm::cross(dir, Vec3(0.0f, 1.0f, 0.0f));
			if (glm::dot(bendDir, bendDir) < 1e-8f) {
				bendDir = glm::cross(dir, Vec3(1.0f, 0.0f, 0.0f));
			}
		}
		bendDir = glm::normalize(bendDir);

		// Law of cosines: angle at the root between the chain axis and the upper bone.
		const float cosRoot = glm::clamp(
			(reach * reach + upper * upper - lower * lower) / (2.0f * reach * upper), -1.0f, 1.0f);
		const float sinRoot = glm::sqrt(glm::max(0.0f, 1.0f - cosRoot * cosRoot));

		const Vec3 newMidPos = rootPos + dir * (upper * cosRoot) + bendDir * (upper * sinRoot);
		const Vec3 newTipPos = rootPos + dir * reach;

		// Two swing deltas: aim the upper bone at the new mid, then the lower bone at the new tip.
		// Applied on the GLOBAL rotations; the tip keeps its incoming global rotation.
		const Quaternion rootDelta = RotationBetween(midPos - rootPos, newMidPos - rootPos);
		const Vec3 tipAfterRootSwing = rootPos + rootDelta * (tipPos - rootPos);
		const Quaternion midDelta = RotationBetween(tipAfterRootSwing - newMidPos, newTipPos - newMidPos);

		BoneTransform rootGlobal = globals[root];
		rootGlobal.Rotation = rootDelta * rootGlobal.Rotation;
		BoneTransform midGlobal = globals[mid];
		midGlobal.Location = newMidPos;
		midGlobal.Rotation = midDelta * rootDelta * midGlobal.Rotation;
		BoneTransform tipGlobal = globals[tip];
		tipGlobal.Location = newTipPos;

		// Back down to local — the pose leaving this node is parent-space, same contract as
		// everything else in the graph. Mid/tip locals come out longer when stretch kicked in,
		// which is exactly how the stretch reaches the skin (translation, not scale).
		const Int32 rootParent = layout.ParentIndex[root];
		const BoneTransform rootParentGlobal =
			(rootParent < 0) ? BoneTransform() : globals[static_cast<UInt64>(rootParent)];
		const BoneTransform newRootLocal = rootParentGlobal.Inverse().Compose(rootGlobal);
		const BoneTransform newMidLocal = rootGlobal.Inverse().Compose(midGlobal);
		const BoneTransform newTipLocal = midGlobal.Inverse().Compose(tipGlobal);

		pose[root] = BlendTransforms(pose[root], newRootLocal, alpha);
		pose[mid] = BlendTransforms(pose[mid], newMidLocal, alpha);
		pose[tip] = BlendTransforms(pose[tip], newTipLocal, alpha);
		pose[root].NormalizeRotation();
		pose[mid].NormalizeRotation();
		pose[tip].NormalizeRotation();
		return pose;
	}
}
