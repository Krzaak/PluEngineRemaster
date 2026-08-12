//
// Created by Plutex on 7/21/26.
//

#include "PluEngine/AssetTypes/AnimationGraph/AnimGraphNode.h"

namespace Plu
{
	Pose AnimGraphNode::EvaluateInputPose(AnimEvalContext& context, const String& pinName) const
	{
		if (context.Graph)
		{
			if (auto* source = dynamic_cast<AnimGraphNode*>(context.Graph->GetLinkSource(Uuid, pinName)))
				return source->Evaluate(context);
		}

		Pose fallback;
		if (context.TargetSkeleton)
			context.TargetSkeleton->GetPoseLayout().MakeBindPose(fallback);
		return fallback;
	}
}
