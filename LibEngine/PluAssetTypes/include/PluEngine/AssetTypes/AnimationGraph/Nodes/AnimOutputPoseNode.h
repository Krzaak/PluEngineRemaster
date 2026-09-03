//
// Created by Plutex on 7/21/26.
//

#ifndef PLUENGINE_ANIMOUTPUTPOSENODE_H
#define PLUENGINE_ANIMOUTPUTPOSENODE_H

#include "PluEngine/AssetTypes/AnimationGraph/AnimGraphNode.h"
#include "AnimOutputPoseNode.generated.h"

namespace Plu
{
	// The graph root: the final pose leaves the graph here. One pose input, no output.
	PLU_STRUCT()
	struct PLUASSETTYPES_API AnimOutputPoseNode : AnimGraphNode
	{
		REFLECTION_BODY_ANIMOUTPUTPOSENODE()

		void BuildPins() override { AddPoseInput("Pose"); }
		String GetDisplayName() override { return "Output Pose"; }
		Pose Evaluate(AnimEvalContext& context) override { return EvaluateInputPose(context, "Pose"); }
	};
}

#endif //PLUENGINE_ANIMOUTPUTPOSENODE_H
