//
// Created by Plutex on 7/20/26.
//

#ifndef PLUENGINE_ANIMATIONGRAPH_H
#define PLUENGINE_ANIMATIONGRAPH_H

#include "PluEngine/Managers/AssetsManager.h"
#include "AnimationGraph.generated.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
    // Node graph that drives a skeleton's pose: samplers, blends, bone masks, blend spaces and
    // state transitions, evaluated per frame into a Pose (see PluEngine/Animation/BoneTransform.h).
    //
    // Boilerplate for now — no nodes, no evaluation. A JSON asset (.pluasset), so both loading and
    // saving ride the reflection-driven path: every PLU_PROPERTY added here is persisted for free.
    PLU_STRUCT()
    struct PLU_API AnimationGraph : IAssetData
    {
        REFLECTION_BODY_ANIMATIONGRAPH()
    };
}

#endif //PLUENGINE_ANIMATIONGRAPH_H
