//
// Created by Plutex on 10.01.2026.
//

#ifndef PLUENGINE_STATICMESH_H
#define PLUENGINE_STATICMESH_H
#include "PluEngine/Managers/AssetsManager.h"

namespace Plu
{
    struct StaticMesh : IAssetInfo
    {
        MaxUInt16 VBO;
        MaxUInt16 VAO;
        MaxUInt16 EBO;
        MaxUInt16 VertexCount;
    };
}

#endif //PLUENGINE_STATICMESH_H