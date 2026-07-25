//
// Created by Plutex on 7/25/26.
//

#ifndef PLUENGINE_ANIMLAYEREDBLENDPERBONENODE_H
#define PLUENGINE_ANIMLAYEREDBLENDPERBONENODE_H

#include "PluEngine/AssetTypes/AnimationGraph/AnimGraphNode.h"
#include "PluEngine/Animation/BoneRef.h"
#include "AnimLayeredBlendPerBoneNode.generated.h"

namespace Plu
{
	// Splits the skeleton in two at Bone: everything OUTSIDE its subtree comes from "A", Bone and its
	// whole subtree come from "B", weighted by Alpha. The classic "upper body from one animation, legs
	// from another" node. Alpha is a hard per-subtree weight (no falloff up the hierarchy) — the
	// transition at the chosen joint is a sharp cut by design, not a smoothed blend.
	PLU_STRUCT()
	struct PLU_API AnimLayeredBlendPerBoneNode : AnimGraphNode
	{
		REFLECTION_BODY_ANIMLAYEREDBLENDPERBONENODE()

		// Root of the layered subtree. Not a pinnable type (see IsPinnableDataType) — details panel only.
		PLU_PROPERTY()
		BoneRef Bone;

		// Fallback while the Alpha pin is unconnected — same pattern as AnimBlendNode::Alpha.
		PLU_PROPERTY()
		float Alpha = 1.0f;

		void BuildPins() override
		{
			AddPoseInput("A");
			AddPoseInput("B");
			AddPoseOutput("Result");
			BuildDataPinsFromReflection(); // adds the "Alpha" float data input pin
		}

		String GetDisplayName() override;

		Pose Evaluate(AnimEvalContext& context) override;
	};
}

#endif //PLUENGINE_ANIMLAYEREDBLENDPERBONENODE_H
