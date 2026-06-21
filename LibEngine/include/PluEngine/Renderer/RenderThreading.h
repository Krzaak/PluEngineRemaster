//
// Created by Plutex on 6/21/26.
//

#ifndef PLUENGINE_RENDERTHREADING_H
#define PLUENGINE_RENDERTHREADING_H

#include "glm/detail/type_quat.hpp"
#include "PluEngine/PluTypes.h"
#include "PluEngine/PluUUID.h"

namespace Plu
{
    enum class RenderObjectType
    {
        STATIC_MESH
    };

    struct RenderObject
    {
        Vec3 Location;
        Quaternion Rotation;
        Vec3 Scale;

        RenderObjectType Type;
    };

    struct StaticMeshRenderObject : RenderObject
    {
        PluUUID MeshUUID;
        PluUUID MaterialUUID;
    };

    struct DirectionalLightRenderObject : RenderObject
    {
        Vec3 Color;
        float Intensity;
        Vec3 Direction;
    };

    //RenderSnapshot
    struct RenderSnapshot
    {
        DynamicArray<StaticMeshRenderObject> StaticMeshRenderObjects;
        DirectionalLightRenderObject DirLight;

        void Clear()
        {
            StaticMeshRenderObjects.Clear();
            DirLight = DirectionalLightRenderObject();
        }
    };
}

#endif //PLUENGINE_RENDERTHREADING_H
