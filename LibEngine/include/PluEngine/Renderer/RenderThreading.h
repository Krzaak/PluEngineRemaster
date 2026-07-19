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
    struct SkeletonNode;
    struct SkeletonBone;

    enum class RenderObjectType
    {
        STATIC_MESH,
        SKELETAL_MESH,
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

    struct SkeletalMeshRenderObject : RenderObject
    {
        PluUUID MeshUUID;
        PluUUID MaterialUUID;
        bool CastsShadow{};

        DynamicArray<std::pair<Matrix4, Matrix4>> Bones;

        SkeletalMeshRenderObject(PluUUID Mesh, PluUUID Material, Vec3 Loc, Quaternion Rot, Vec3 Scl, Matrix4 MdlMatrix, bool Shadow, DynamicArray<std::pair<Matrix4, Matrix4>>* bones) : RenderObject()
        {
            ModelMatrix = MdlMatrix;
            Location = Loc;
            Rotation = Rot;
            Scale = Scl;
            Type = RenderObjectType::SKELETAL_MESH;
            MeshUUID = Mesh;
            MaterialUUID = Material;
            CastsShadow = Shadow;
            Bones = *bones;
        }
    };

    struct DirectionalLightRenderObject : RenderObject
    {
        Vec3 Color;
        float Intensity;
        Vec3 Direction;
    };

    // Layout MUSI odpowiadać `struct InstanceData` w shaderach instanced (BasicVertInstanced.vert,
    // OnlyPositionInstanced.vert). NormalMatrix jest mat4, nie mat3: std430 daje tablicy mat3
    // stride 48 B, a glm::mat3 ma 36 B w C++ — surowy upload rozjechałby się od drugiego elementu.
    struct InstanceGPUData
    {
        Matrix4 ModelMatrix;   //  0, 64 B
        Matrix4 NormalMatrix;  // 64, 64 B — transpose(inverse(model)), liczone na CPU
    };
    static_assert(sizeof(InstanceGPUData) == 128);

    // Bounds instancji dla frustum cullingu, równoległa do StaticInstanceData (ten sam indeks).
    struct InstanceCullData
    {
        Vec3  BoundsCenter;   // world space
        float BoundsRadius;   // sfera, nie AABB — niezmiennicza na rotację, 1 dot na płaszczyznę
    };

    // Klucz = (MeshUUID, MaterialUUID, CastsShadow). CastsShadow w kluczu sprawia, że batch jest
    // jednorodny i shadow pass reużywa ten sam ciągły zakres instancji.
    // Kolejność instancji w batchu jest nośna: najpierw widoczne z kamery [Offset, Offset+VisibleCount),
    // potem odrzucone przez culling ale rzucające cień [Offset+VisibleCount, Offset+TotalCount).
    // VisibleCount <= TotalCount zawsze; instancje ani widoczne, ani rzucające cienia nie trafiają
    // do bufora wcale.
    struct StaticMeshBatch
    {
        PluUUID MeshUUID, MaterialUUID;
        UInt32  InstanceOffset = 0;
        UInt32  VisibleCount   = 0;  // główny pass: [Offset, Offset + VisibleCount)
        UInt32  TotalCount     = 0;  // shadow pass: [Offset, Offset + TotalCount)
        bool    CastsShadow    = false;
    };

    //RenderSnapshot
    struct RenderSnapshot
    {
        DynamicArray<SkeletalMeshRenderObject> SkeletalMeshRenderObjects;

        // Batching instancingu static meshy (grupowanie na wątku MAIN w RenderSnapshotBuilder).
        // StaticInstanceData indeksowana przez gl_InstanceID na GPU (SSBO, binding 1);
        // StaticInstanceBounds równoległa, tylko do cullingu (nieuploadowana na GPU).
        DynamicArray<StaticMeshBatch> StaticMeshBatches;
        DynamicArray<InstanceGPUData> StaticInstanceData;
        DynamicArray<InstanceCullData> StaticInstanceBounds;
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

        // Liczniki diagnostyczne bieżącej klatki (panel Render/GPU). Wypełniane przez Renderer
        // NA WĄTKU RENDERU podczas faktycznego rysowania — odzwierciedlają realne draw calle
        // (po batchowaniu/cullingu), nie tylko liczbę obiektów w snapshocie. Panel (main thread)
        // nie czyta tych pól bezpośrednio (wyścig z render threadem) — Renderer mirroruje
        // finalne wartości przez SetRenderFrameStats/Get* (PluUtils.h), analogicznie do FPS.
        UInt32 StatDrawCalls = 0;
        UInt32 StatInstancesDrawn = 0;
        UInt32 StatCulledCount = 0;

        void Clear()
        {
            SkeletalMeshRenderObjects.Clear();
            StaticMeshBatches.Clear();
            StaticInstanceData.Clear();
            StaticInstanceBounds.Clear();
            DirLight = DirectionalLightRenderObject();
            HasDirLight = false;
            CameraProjectionMatrix = Matrix4();
            CameraLocation = Vec3();
            CameraRotation = Vec3();
            CameraFOV = 45.0f;
            DebugLineVerts.Clear();
            DebugPointVerts.Clear();
            DebugPointSize = 10.0f;
            StatDrawCalls = 0;
            StatInstancesDrawn = 0;
            StatCulledCount = 0;
        }
    };
}

#endif //PLUENGINE_RENDERTHREADING_H
