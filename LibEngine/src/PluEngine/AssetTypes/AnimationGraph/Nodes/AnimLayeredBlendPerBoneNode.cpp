//
// Created by Plutex on 7/25/26.
//

#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimLayeredBlendPerBoneNode.h"

namespace Plu
{
	String AnimLayeredBlendPerBoneNode::GetDisplayName()
	{
		return Bone.Name.IsEmpty() ? "Layered Blend per Bone" : "Layered Blend per Bone (" + Bone.Name + ")";
	}

	Pose AnimLayeredBlendPerBoneNode::Evaluate(AnimEvalContext& context)
	{
		PLU_PROFILE_SCOPE("AnimLayeredBlendPerBone");

		const float alpha = glm::clamp(ReadDataPin<float>(context, "Alpha", Alpha), 0.0f, 1.0f);

		// alpha <= 0, or no skeleton to resolve the bone against: "B" never needs evaluating — it can
		// be a whole animation sub-tree (same reasoning as AnimBlendByBoolNode's untaken branch).
		if (alpha <= 0.0f || !context.TargetSkeleton) {
			return EvaluateInputPose(context, "A");
		}

		const SkeletonPoseLayout& layout = context.TargetSkeleton->GetPoseLayout();
		const Int32 root = layout.FindIndex(Bone.Name);
		if (root < 0) {
			return EvaluateInputPose(context, "A");
		}

		const Pose a = EvaluateInputPose(context, "A");
		const Pose b = EvaluateInputPose(context, "B");

		DynamicArray<float> weights;
		layout.BuildSubtreeMask(root, alpha, 0.0f, weights);

		Pose result;
		BlendPosesMasked(a, b, weights, 0.0f, result);
		return result;
	}
}
