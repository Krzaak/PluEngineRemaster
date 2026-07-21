//
// Created by Plutex on 7/21/26.
//

#ifndef PLUENGINE_ANIMGRAPHNODE_H
#define PLUENGINE_ANIMGRAPHNODE_H

#include "PluEngine/NodeGraph/GraphNode.h"
#include "PluEngine/Animation/BoneTransform.h" // Pose
#include "AnimGraphNode.generated.h"

namespace Plu
{
	// Context threaded through a graph evaluation. Minimal for now — grows with the runtime
	// (skeleton, play time, per-instance state). Not reflected.
	struct AnimEvalContext
	{
		float TimeSeconds = 0.0f;
	};

	// Category base for animation-graph nodes. Their flow pins carry poses (flow TypeId "Pose"),
	// and evaluation produces a Pose. Concrete nodes derive this. Abstract → not offered in palettes.
	PLU_STRUCT(Abstract)
	struct PLU_API AnimGraphNode : GraphNode
	{
		REFLECTION_BODY_ANIMGRAPHNODE()

		// Flow kind for pose wires. A pose output only connects to a pose input (NodePin::CanConnect).
		static constexpr const char* PoseFlow = "Pose";

		// Evaluated by the graph runtime (traversal follows pose links to upstream nodes). Default:
		// empty pose. The traversal/evaluation runtime is a later phase — nodes stub this for now.
		virtual Pose Evaluate(AnimEvalContext& context) { return Pose(); }

	protected:
		void AddPoseInput(const String& name)  { AddPin(name, EPinDirection::Input,  EPinCategory::Flow, PoseFlow); }
		void AddPoseOutput(const String& name) { AddPin(name, EPinDirection::Output, EPinCategory::Flow, PoseFlow); }
	};
}

#endif //PLUENGINE_ANIMGRAPHNODE_H
