//
// Created by Plutex on 10.01.2026.
//

#ifndef PLUENGINE_STATICMESH_H
#define PLUENGINE_STATICMESH_H
#include "PluEngine/PluTypes.h"
#include "PluEngine/Managers/AssetsManager.h"

namespace Plu
{
    struct Vertex
    {
        Vec3 Position;
        MaxUInt32 Normal;
        MaxUInt16 UV[2];
        MaxUInt32 Color;
    };
    struct MeshData
    {
        DynamicArray<Vertex> Vertices;
        DynamicArray<MaxUInt32> Indices;
        MaxUInt16 MaterialIndex;
    };
    struct StaticMesh : IAssetInfo
    {
        MaxUInt16 VBO;
        MaxUInt16 VAO;
        MaxUInt16 EBO;
        MaxUInt16 VertexCount;
    };
}

#endif //PLUENGINE_STATICMESH_H