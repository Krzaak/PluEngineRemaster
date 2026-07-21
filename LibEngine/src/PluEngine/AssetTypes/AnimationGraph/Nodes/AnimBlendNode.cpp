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

		Pose result;
		BlendPoses(a, b, Alpha, result);
		return result;
	}
}
