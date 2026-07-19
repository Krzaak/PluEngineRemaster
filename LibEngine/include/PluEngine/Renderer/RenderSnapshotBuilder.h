//
// Created by Plutex on 6/21/26.
//

#ifndef PLUENGINE_RENDERSNAPSHOTBUILDER_H
#define PLUENGINE_RENDERSNAPSHOTBUILDER_H

#include "PluEngine/Objects/EngineObject.h"
#include "RenderSnapshotBuilder.generated.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/Renderer/RenderThreading.h"
#include "PluEngine/Threading/TripleBuffer.h"

namespace Plu
{
    class CameraComponent;
    class IRendererCamera;
    struct RenderSnapshot;


    PLU_CLASS()
    class PLU_API RenderSnapshotBuilder : public EngineObject
    {
        REFLECTION_BODY_RENDERSNAPSHOTBUILDER()
    private:
        TripleBuffer<RenderSnapshot*>* mTripleBuffer;
        ApplicationInfo* mAppInfo;

        // Batching instancingu static meshy: hash klucza (MeshUUID, MaterialUUID, CastsShadow) ->
        // indeksy w RenderSnapshot::StaticMeshBatches. Member (nie lokalna per klatka) — Clear()
        // na starcie każdego builda, reużywany bufor zamiast realokacji co klatkę. Kolizje: małe
        // kubełki porównywane po pełnym kluczu w BuildSnapshotAndPublish.
        GameHashMap<UInt64, DynamicArray<UInt32>> mBatchLookup;

        // Scratch dla dwuprzebiegowej emisji batchy (bucketing -> prefix-sum + sort batchy ->
        // scatter). Member, reużywany między klatkami — unika alokacji DynamicArray per batch.
        struct StaticMeshBatchScratchEntry
        {
            UInt32 BatchIndex;
            InstanceGPUData Instance;
            InstanceCullData Bounds;
        };
        DynamicArray<StaticMeshBatchScratchEntry> mBatchScratch;

        [[nodiscard]] Matrix4 GetProjectionMatrix(IRendererCamera* camera) const;
        [[nodiscard]] Matrix4 GetViewMatrix(IRendererCamera* camera) const;
    public:
        RenderSnapshotBuilder();
        RenderSnapshotBuilder(TripleBuffer<RenderSnapshot*>* tripleBuffer, ApplicationInfo* applicationInfo);
        virtual ~RenderSnapshotBuilder() override;

        static Matrix4 GetLastFrameProjectionMatrix();
        static Matrix4 GetLastFrameViewMatrix();

        void BuildSnapshotAndPublish(float deltaTime = 0.0f);
    };
}

#endif //PLUENGINE_RENDERSNAPSHOTBUILDER_H
