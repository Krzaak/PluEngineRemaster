//
// Created by Plutex on 7/21/26.
//

#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimOutputPoseNode.h"

namespace Plu
{
	GameHashMap<String, AnimationGraphVariableFactory::FactoryFunc>& AnimationGraphVariableFactory::GetFactoryMap()
	{
		static GameHashMap<String, FactoryFunc> FactoryMap;
		return FactoryMap;
	}

	bool AnimationGraph::IsCompatibleWith(const TUsePointer<Skeleton>& skeleton) const
	{
		if (!TargetSkeleton) return true;
		if (!skeleton) return false;
		return TargetSkeleton->Uuid.getUUID() == skeleton->Uuid.getUUID();
	}

	Pose AnimationGraph::Evaluate(AnimEvalContext& context)
	{
		context.Graph = this;
		// Callers that know the concrete mesh (RenderSnapshotBuilder) pass its skeleton; anyone
		// evaluating the asset on its own (editor preview) gets the graph's authoring skeleton.
		if (!context.TargetSkeleton)
			context.TargetSkeleton = TargetSkeleton;
		for (TOwningPointer<GraphNode>& node : Nodes)
		{
			if (auto* output = dynamic_cast<AnimOutputPoseNode*>(node.GetRaw()))
				return output->Evaluate(context);
		}
		return Pose();
	}
}
