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
        Matrix4 ModelMatrix;

        RenderObjectType Type;
    };

    struct StaticMeshRenderObject : RenderObject
    {
        PluUUID MeshUUID;
        PluUUID MaterialUUID;
        bool CastsShadow{};

        StaticMeshRenderObject(PluUUID Mesh, PluUUID Material, Vec3 Loc, Quaternion Rot, Vec3 Scl, Matrix4 MdlMatrix, bool Shadow) : RenderObject()
        {
            ModelMatrix = MdlMatrix;
            Location = Loc;
            Rotation = Rot;
            Scale = Scl;
            Type = RenderObjectType::STATIC_MESH;
            MeshUUID = Mesh;
            MaterialUUID = Material;
            CastsShadow = Shadow;
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
        bool HasDirLight = false;

        Matrix4 CameraProjectionMatrix;
        Vec3 CameraLocation;
        Vec3 CameraRotation;
        // Pole widzenia kamery (stopnie) — potrzebne na wątku renderu do zbudowania
        // pod-frustumów kaskad cieni (CSM). Projekcja sama nie wystarcza, bo CSM
        // przelicza near/far per-kaskada.
        float CameraFOV = 45.0f;

        // Geometria debugowa fizyki, wyekstrahowana na MAIN (Jolt + ObjectManager są
        // main-only) i spakowana do płaskich buforów interleaved pos(3)+color(3).
        // Wątek renderu tylko uploaduje je do VBO i rysuje shaderem DebugLine.
        DynamicArray<float> DebugLineVerts;   // GL_LINES,  6 floatów / wierzchołek
        DynamicArray<float> DebugPointVerts;  // GL_POINTS, 6 floatów / wierzchołek
        float DebugPointSize = 10.0f;

        void Clear()
        {
            StaticMeshRenderObjects.Clear();
            DirLight = DirectionalLightRenderObject();
            HasDirLight = false;
            CameraProjectionMatrix = Matrix4();
            CameraLocation = Vec3();
            CameraRotation = Vec3();
            CameraFOV = 45.0f;
            DebugLineVerts.Clear();
            DebugPointVerts.Clear();
            DebugPointSize = 10.0f;
        }
    };
}

#endif //PLUENGINE_RENDERTHREADING_H
