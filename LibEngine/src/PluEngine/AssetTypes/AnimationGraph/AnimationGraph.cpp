//
// Created by Plutex on 7/21/26.
//

#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimOutputPoseNode.h"
#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimVariableNode.h"

namespace Plu
{
	GameHashMap<String, AnimationGraphVariableFactory::VariableTypeInfo>& AnimationGraphVariableFactory::GetFactoryMap()
	{
		static GameHashMap<String, VariableTypeInfo> FactoryMap;
		return FactoryMap;
	}

	TUsePointer<IAnimationGraphVariable> AnimationGraph::FindVariable(const String& name)
	{
		for (TOwningPointer<IAnimationGraphVariable>& variable : Variables) {
			if (variable && variable->Name == name) return variable;
		}
		return nullptr;
	}

	AnimVariableNode* AnimationGraph::AddVariableNode(const TUsePointer<IAnimationGraphVariable>& variable)
	{
		if (!variable) return nullptr;
		auto* node = static_cast<AnimVariableNode*>(AnimVariableNode::GetStaticClass()->Construct());
		if (!node) return nullptr;
		node->Variable     = variable;
		node->VariableName = variable->Name;
		node->BuildPins();
		Nodes.PushBack(TOwningPointer<GraphNode>(node));
		return node;
	}

	void AnimationGraph::ResolveVariableReferences()
	{
		for (TOwningPointer<GraphNode>& node : Nodes) {
			auto* variableNode = dynamic_cast<AnimVariableNode*>(node.GetRaw());
			if (!variableNode) continue;
			variableNode->Variable = FindVariable(variableNode->VariableName);
			// Pins were already built once during load (with no bound variable, so an untyped pin);
			// clear before rebuilding or the pin would be duplicated.
			variableNode->InputPins.Clear();
			variableNode->OutputPins.Clear();
			variableNode->BuildPins();
		}
	}

	void AnimationGraph::SyncVariableNodeNames()
	{
		for (TOwningPointer<GraphNode>& node : Nodes) {
			auto* variableNode = dynamic_cast<AnimVariableNode*>(node.GetRaw());
			if (variableNode && variableNode->Variable) {
				variableNode->VariableName = variableNode->Variable->Name;
			}
		}
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
