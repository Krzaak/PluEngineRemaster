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
        DIRECTIONAL_LIGHT,
        SPOT_LIGHT
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

        // World-space bounding sphere, used to cull the mesh per shadow cascade. Derived from the
        // component's BIND-POSE bounds and inflated on MAIN, because animation moves vertices
        // outside them — a tight sphere would pop limbs' shadows in and out mid-animation.
        Vec3  BoundsCenter{};
        float BoundsRadius{};

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

    // Shadow settings authored on the DirectionalLight (main thread) and consumed by the
    // renderer (render thread). Plain POD — it travels in the snapshot like everything else,
    // so no engine object is touched across the thread boundary. The renderer clamps every
    // field; the values here are whatever the user typed into the details panel.
    struct DirectionalLightShadowSettings
    {
        bool  CastShadows   = true;
        float ShadowDistance = 150.0f;
        Int32 CascadeCount   = 4;
        float SplitLambda    = 0.9f;
        Int32 Resolution     = 2048;   // nearest cascade; the rest follow ResolutionFalloff
        Int32 ResolutionFalloff = 2;   // halve the resolution every N cascades (0 = uniform)
        float NormalBias     = 1.0f;   // texels
        float DepthBias      = 0.005f; // metres
        float PcfRadius      = 1.5f;   // texels
        bool  PcfAutoTaps    = true;   // derive the tap count from PcfRadius (ignores PcfTapCount)
        Int32 PcfTapCount    = 8;      // samples in the PCF disk, when PcfAutoTaps is off
        bool  PcfRotate      = true;   // rotate the disk per pixel
        float CascadeBlend   = 0.15f;  // fraction of a cascade

        // Contact shadows — screen-space ray march against the depth prepass (see DirectionalLight).
        bool  ContactShadows          = true;
        float ContactShadowLength     = 0.25f;  // metres of world space marched
        Int32 ContactShadowSteps      = 16;     // samples along the ray (quadratically spaced)
        float ContactShadowThickness  = 0.05f;  // metres; assumed occluder depth
        float ContactShadowBias       = 0.002f; // metres; keeps a surface off its own ray
    };

    struct DirectionalLightRenderObject : RenderObject
    {
        Vec3 Color;
        float Intensity;
        Vec3 Direction;
        DirectionalLightShadowSettings Shadow;
    };

    // One spot light of the frame, already frustum-culled on MAIN. Plain POD, like every other
    // snapshot entry — no engine object crosses the thread boundary.
    //
    // The cosines are precomputed here rather than in the shader because they are per light, not
    // per fragment: a scene with 64 spots would otherwise pay two cos() per light per pixel.
    struct SpotLightRenderObject : RenderObject
    {
        Vec3  Color{};
        float Intensity = 0.0f;
        Vec3  Direction{};        // direction of travel (light forward), normalised
        float Range = 0.0f;       // metres; also the shadow projection's far plane
        float InnerConeCos = 0.0f;
        float OuterConeCos = 0.0f;
        float OuterConeAngle = 0.0f;  // radians, HALF angle — sizes the shadow projection's FOV

        bool  CastShadows = false;
        float ShadowDepthBias = 0.0f;   // [0,1] projected depth
        float ShadowNormalBias = 0.0f;  // texels
        float ShadowPcfRadius = 0.0f;   // texels
        Int32 ShadowPcfTaps = 0;
        Int32 ShadowPriority = 0;

        // Stable tiebreak when two lights score identically, so the importance sort (and with it
        // the atlas slot assignment) does not flip between frames on a coin toss.
        PluUUID LightUUID;
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

        // Spot lights visible from the camera this frame, ALREADY SORTED descending by importance
        // (see RenderSnapshotBuilder::CollectSpotLights) and trimmed to kMaxVisibleSpotLights.
        // The render thread hands out shadow atlas slots by walking this array front to back, so
        // it never has to sort anything itself.
        DynamicArray<SpotLightRenderObject> SpotLights;

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

        // Editor grid (infinite, procedural — EditorGrid.frag on the Y=0 plane, fixed 1 m
        // cells). View-only editor setting copied from SceneWorld on MAIN; the render thread
        // draws a fullscreen pass (Renderer::RenderEditorGrid) when enabled.
        bool ShowEditorGrid = false;

        // Shadow cascade debug tint (View -> scene settings). Mirrors SceneWorld::ShowShadowCascades;
        // the renderer forwards it into ShadowData::DebugVisualizeCascades.
        bool ShowShadowCascades = false;

        // Liczniki diagnostyczne bieżącej klatki (panel Render/GPU). Wypełniane przez Renderer
        // NA WĄTKU RENDERU podczas faktycznego rysowania — odzwierciedlają realne draw calle
        // (po batchowaniu/cullingu), nie tylko liczbę obiektów w snapshocie. Panel (main thread)
        // nie czyta tych pól bezpośrednio (wyścig z render threadem) — Renderer mirroruje
        // finalne wartości przez SetRenderFrameStats/Get* (PluUtils.h), analogicznie do FPS.
        UInt32 StatDrawCalls = 0;
        UInt32 StatInstancesDrawn = 0;
        UInt32 StatCulledCount = 0;

        bool IsSnapshotValid = false;

        void Clear()
        {
            SkeletalMeshRenderObjects.Clear();
            StaticMeshBatches.Clear();
            StaticInstanceData.Clear();
            StaticInstanceBounds.Clear();
            DirLight = DirectionalLightRenderObject();
            HasDirLight = false;
            SpotLights.Clear();
            CameraProjectionMatrix = Matrix4();
            CameraLocation = Vec3();
            CameraRotation = Vec3();
            CameraFOV = 45.0f;
            DebugLineVerts.Clear();
            DebugPointVerts.Clear();
            DebugPointSize = 10.0f;
            ShowEditorGrid = false;
            ShowShadowCascades = false;
            StatDrawCalls = 0;
            StatInstancesDrawn = 0;
            StatCulledCount = 0;
            IsSnapshotValid = false;
        }
    };
}

#endif //PLUENGINE_RENDERTHREADING_H
