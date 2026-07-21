//
// Created by Plutex on 7/20/26.
//

#ifndef PLUENGINE_ANIMATIONGRAPH_H
#define PLUENGINE_ANIMATIONGRAPH_H

#include "PluEngine/NodeGraph/NodeGraph.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimGraphNode.h"
#include "AnimationGraph.generated.h"

namespace Plu
{
    struct TypeInfo;

    // Node graph that drives a skeleton's pose: samplers, blends, bone masks, blend spaces and
    // state transitions, evaluated into a Pose (see PluEngine/Animation/BoneTransform.h).
    //
    // The reusable node-graph machinery lives in PluEngine/NodeGraph/ (GraphNode/NodeGraph/links +
    // NodeGraphSerializer). This asset just specialises it to the animation domain: its nodes derive
    // AnimGraphNode. Nodes + links are (de)serialised by NodeGraphSerializer from the asset loader.
    PLU_STRUCT()
    struct PLU_API AnimationGraph : NodeGraph
    {
        REFLECTION_BODY_ANIMATIONGRAPH()

        // A graph is authored against one skeleton: node indices in every Pose flowing through it
        // are SkeletonPoseLayout indices of this skeleton, so a graph is only meaningful on meshes
        // sharing it. Used as the evaluation skeleton when the caller doesn't supply one (editor
        // preview), and as the reference for node-side authoring (bone pickers, masks).
        PLU_PROPERTY()
        TUsePointer<Skeleton> TargetSkeleton;

        // Whether this graph may drive `skeleton`. True when TargetSkeleton is unassigned (an
        // unconstrained graph — the callers then supply whatever skeleton they have) or when the
        // two are the same asset. False means the graph's node indices address a different
        // skeleton, so evaluating it would produce a garbage pose.
        [[nodiscard]] bool IsCompatibleWith(const TUsePointer<Skeleton>& skeleton) const;

        // The "Add Node" palette lists non-abstract subclasses of this.
        TypeInfo* GetNodeBaseType() override { return AnimGraphNode::GetStaticClass(); }

        // Evaluates the graph: finds the AnimOutputPoseNode and pulls its pose, which recursively
        // pulls upstream nodes across pose links (AnimGraphNode::EvaluateInputPose). Empty pose
        // when the graph has no output node.
        Pose Evaluate(AnimEvalContext& context);
    };
}

#endif //PLUENGINE_ANIMATIONGRAPH_H
