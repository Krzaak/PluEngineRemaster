//
// Created by Plutex on 7/21/26.
//

#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimBlendNode.h"

namespace Plu
{
	Pose AnimBlendNode::Evaluate(AnimEvalContext& context)
	{
		const Pose a = EvaluateInputPose(context, "A");
		const Pose b = EvaluateInputPose(context, "B");
		const float alpha = ReadDataPin<float>(context, "Alpha", Alpha);

		Pose result;
		BlendPoses(a, b, alpha, result);
		return result;
	}
}
