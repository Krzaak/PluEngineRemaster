//
// Created by Plutex on 7/21/26.
//

#ifndef PLUENGINE_ANIMSAMPLENODE_H
#define PLUENGINE_ANIMSAMPLENODE_H

#include "PluEngine/AssetTypes/AnimationGraph/AnimGraphNode.h"
#include "PluEngine/AssetTypes/Animation/SkeletalAnimation.h" // Animation
#include "AnimSampleNode.generated.h"

namespace Plu
{
	// Samples an animation asset into a pose at the evaluation time. The animation reference is an
	// asset UUID (edited via the asset picker in the details panel), not a graph pin.
	PLU_STRUCT()
	struct PLUASSETTYPES_API AnimSampleNode : AnimGraphNode
	{
		REFLECTION_BODY_ANIMSAMPLENODE()

		PLU_PROPERTY()
		TUsePointer<Animation> AnimationAsset;

		void BuildPins() override { AddPoseOutput("Pose"); }
		String GetDisplayName() override;
		Pose Evaluate(AnimEvalContext& context) override;
	};
}

#endif //PLUENGINE_ANIMSAMPLENODE_H
