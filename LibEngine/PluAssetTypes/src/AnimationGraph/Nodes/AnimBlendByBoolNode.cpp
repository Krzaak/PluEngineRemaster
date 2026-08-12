//
// Created by Plutex on 7/25/26.
//

#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimBlendByBoolNode.h"

namespace Plu
{
	Pose AnimBlendByBoolNode::Evaluate(AnimEvalContext& context)
	{
		const bool condition = ReadDataPin<bool>(context, "Condition", Condition);
		// Only the taken branch is evaluated — unlike the data-side LogicSelectNode, where both sides
		// are cheap. Here the losing branch could be a whole animation sub-tree.
		return EvaluateInputPose(context, condition ? "True" : "False");
	}
}
