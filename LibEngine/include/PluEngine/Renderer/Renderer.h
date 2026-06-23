//
// Created by Plutex on 6/22/26.
//

#ifndef PLUENGINE_RENDERER_H
#define PLUENGINE_RENDERER_H

#include "GLFrameBuffer.h"
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"

namespace Plu
{
    struct RenderSnapshot;

    //This is a render thread only object
    class Renderer
    {
        ApplicationInfo* mApplicationInfo;
        TOwningPointer<FrameBuffer> mMainBuffer;
    public:
        Renderer() = default;
        ~Renderer() = default;

        void Initialize(ApplicationInfo* applicationInfo);
        void RenderSnapshot(RenderSnapshot* snapshot);
        void Shutdown();
    };
}

#endif //PLUENGINE_RENDERER_H
