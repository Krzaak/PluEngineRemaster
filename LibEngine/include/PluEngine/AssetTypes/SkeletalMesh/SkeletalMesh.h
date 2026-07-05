//
// Created by Plutex on 7/5/26.
//

#ifndef PLUENGINE_SKELETALMESH_H
#define PLUENGINE_SKELETALMESH_H
#include "PluEngine/Managers/AssetsManager.h"
#include "SkeletalMesh.generated.h"

namespace Plu
{
    PLU_STRUCT()
    class SkeletalMesh : public IAssetData
    {
        REFLECTION_BODY_SKELETALMESH()
    };
}

#endif //PLUENGINE_SKELETALMESH_H
