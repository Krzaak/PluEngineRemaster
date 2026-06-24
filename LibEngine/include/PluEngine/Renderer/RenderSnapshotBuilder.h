//
// Created by Plutex on 6/21/26.
//

#ifndef PLUENGINE_RENDERSNAPSHOTBUILDER_H
#define PLUENGINE_RENDERSNAPSHOTBUILDER_H

#include "PluEngine/Objects/EngineObject.h"
#include "RenderSnapshotBuilder.generated.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/Threading/TripleBuffer.h"

namespace Plu
{
    class CameraComponent;
    class IRendererCamera;
    struct RenderSnapshot;


    PLU_CLASS()
    class RenderSnapshotBuilder : public EngineObject
    {
        REFLECTION_BODY_RENDERSNAPSHOTBUILDER()
    private:
        TripleBuffer<RenderSnapshot*>* mTripleBuffer;
        ApplicationInfo* mAppInfo;

        [[nodiscard]] Matrix4 GetProjectionMatrix(IRendererCamera* camera) const;
        [[nodiscard]] Matrix4 GetViewMatrix(IRendererCamera* camera) const;
    public:
        RenderSnapshotBuilder();
        RenderSnapshotBuilder(TripleBuffer<RenderSnapshot*>* tripleBuffer, ApplicationInfo* applicationInfo);
        virtual ~RenderSnapshotBuilder() override;

        void BuildSnapshotAndPublish();
    };
}

#endif //PLUENGINE_RENDERSNAPSHOTBUILDER_H
