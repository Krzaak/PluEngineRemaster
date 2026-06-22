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
        STATIC_MESH,
        DIRECTIONAL_LIGHT
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

        StaticMeshRenderObject() = default;

        StaticMeshRenderObject(PluUUID Mesh, PluUUID Material, Vec3 Loc, Quaternion Rot, Vec3 Scl)
        {
            Location = Loc;
            Rotation = Rot;
            Scale = Scl;
            Type = RenderObjectType::STATIC_MESH;
            MeshUUID = Mesh;
            MaterialUUID = Material;
        }
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

        Matrix4 CameraProjectionMatrix;
        Vec3 CameraLocation;
        Vec3 CameraRotation;

        void Clear()
        {
            StaticMeshRenderObjects.Clear();
            DirLight = DirectionalLightRenderObject();
            CameraProjectionMatrix = Matrix4();
            CameraLocation = Vec3();
            CameraRotation = Vec3();
        }
    };
}

#endif //PLUENGINE_RENDERTHREADING_H
