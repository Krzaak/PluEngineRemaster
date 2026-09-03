//
// Created by Plutex on 7/26/26.
//

#ifndef PLUENGINE_ANIMTWOBONEIKNODE_H
#define PLUENGINE_ANIMTWOBONEIKNODE_H

#include "PluEngine/AssetTypes/AnimationGraph/AnimGraphNode.h"
#include "PluEngine/AssetTypes/Animation/BoneRef.h"
#include "AnimTwoBoneIKNode.generated.h"

namespace Plu
{
	// Space the IK goal positions (EffectorLocation / JointTargetLocation) are authored in.
	// Deliberately no Local here — a parent-space goal position is meaningless for an IK chain.
	PLU_ENUM(PyNamespace=Plu)
	enum class EIKGoalSpace : UInt8
	{
		Component, // skeleton-space (root-relative)
		World,     // world-space, via AnimEvalContext::ComponentToWorld
		// Offset in the posed frame of a reference node (EffectorBone / JointTargetBone),
		// resolved against THIS evaluation's pre-solve globals. The lag-free way to follow a
		// prop riding another bone (left hand gripping a gun carried in the right hand): a
		// World goal read in OnPreEvaluateAnimGraph is one frame stale by construction, but
		// the prop moves rigidly with its bone, so the same reading re-expressed in that
		// bone's space is frame-invariant — see SkeletalMeshComponent::WorldLocationToNodeSpace.
		Bone
	};

	// Classic two-bone IK (shoulder–elbow–hand, hip–knee–foot): pick the tip bone, the chain is
	// the tip's parent (middle joint) and grandparent (chain root). Solves the root and middle
	// rotations so the tip lands on EffectorLocation, bending toward JointTargetLocation (the pole
	// target). The tip keeps its component-space rotation from the incoming pose. Results are
	// written back as LOCAL transforms, so descendants of the tip ride along as usual.
	PLU_STRUCT()
	struct PLUASSETTYPES_API AnimTwoBoneIKNode : AnimGraphNode
	{
		REFLECTION_BODY_ANIMTWOBONEIKNODE()

		// Tip of the chain (e.g. a hand); its parent and grandparent are the joints being solved.
		// Not a pinnable type (see IsPinnableDataType) — details panel only.
		PLU_PROPERTY()
		BoneRef Bone;

		// Where the tip should end up.
		PLU_PROPERTY()
		Vec3 EffectorLocation = Vec3(0.0f);

		PLU_PROPERTY()
		EIKGoalSpace EffectorSpace = EIKGoalSpace::Component;

		// Reference node for EffectorSpace == Bone; ignored in the other spaces. Resolved against
		// the pre-solve pose — a reference inside the solved chain reads the incoming animation,
		// not the IK result.
		PLU_PROPERTY()
		BoneRef EffectorBone;

		// Derive the bend plane from the incoming pose instead of JointTargetLocation — the chain
		// keeps bending the way the animation already bends it. Natural for joints that are always
		// somewhat bent (knees); for a chain the animation nearly straightens (a reaching arm) the
		// plane becomes ill-defined and can flip between frames — that is what an explicit pole
		// target is for, so switch this off there.
		PLU_PROPERTY()
		bool AutoJointTarget = true;

		// Pole target: the middle joint bends toward this point, which fixes the chain's bend
		// plane. Ignored while AutoJointTarget is on. On the chain axis the current bend plane
		// from the incoming pose is kept instead.
		PLU_PROPERTY()
		Vec3 JointTargetLocation = Vec3(0.0f);

		PLU_PROPERTY()
		EIKGoalSpace JointTargetSpace = EIKGoalSpace::Component;

		// Reference node for JointTargetSpace == Bone; ignored in the other spaces. A missing
		// bone falls back to the pose-derived bend plane instead of dropping the whole solve.
		PLU_PROPERTY()
		BoneRef JointTargetBone;

		// When the effector is out of reach, lengthen both bones (via translation, not scale — no
		// skinning artifacts) up to MaxStretchRatio × their rest length instead of just
		// straightening the chain.
		PLU_PROPERTY()
		bool AllowStretch = false;

		// Upper bound on the stretched chain length, as a multiple of the unstretched length.
		PLU_PROPERTY()
		float MaxStretchRatio = 1.5f;

		// Fallback while the Alpha pin is unconnected — same pattern as AnimBlendNode::Alpha.
		PLU_PROPERTY()
		float Alpha = 1.0f;

		void BuildPins() override
		{
			AddPoseInput("Pose");
			AddPoseOutput("Result");
			// Adds "EffectorLocation"/"AutoJointTarget"/"JointTargetLocation"/"AllowStretch"/
			// "MaxStretchRatio"/"Alpha".
			BuildDataPinsFromReflection();
		}

		String GetDisplayName() override;

		Pose Evaluate(AnimEvalContext& context) override;
	};
}

#endif //PLUENGINE_ANIMTWOBONEIKNODE_H
