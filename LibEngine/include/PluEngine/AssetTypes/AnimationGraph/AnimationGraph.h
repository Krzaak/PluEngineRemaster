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

        // The "Add Node" palette lists non-abstract subclasses of this.
        TypeInfo* GetNodeBaseType() override { return AnimGraphNode::GetStaticClass(); }
    };
}

#endif //PLUENGINE_ANIMATIONGRAPH_H
