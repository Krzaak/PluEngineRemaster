//
// Created by Plutex on 7/21/26.
//

#ifndef PLUENGINE_ANIMBLENDNODE_H
#define PLUENGINE_ANIMBLENDNODE_H

#include "PluEngine/AssetTypes/AnimationGraph/AnimGraphNode.h"
#include "AnimBlendNode.generated.h"

namespace Plu
{
	// Blends two poses by Alpha (0 -> A, 1 -> B). Alpha is both an editable parameter (details panel)
	// and, via BuildDataPinsFromReflection(), a float data input pin — so it can be driven by a wire.
	// Registered with a custom NodeView in the editor (the first visual "smaczek").
	PLU_STRUCT()
	struct PLU_API AnimBlendNode : AnimGraphNode
	{
		REFLECTION_BODY_ANIMBLENDNODE()

		PLU_PROPERTY()
		float Alpha = 0.5f;

		void BuildPins() override
		{
			AddPoseInput("A");
			AddPoseInput("B");
			AddPoseOutput("Result");
			BuildDataPinsFromReflection(); // adds the "Alpha" float data input pin
		}
		String GetDisplayName() override { return "Blend"; }
	};
}

#endif //PLUENGINE_ANIMBLENDNODE_H
