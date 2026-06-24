//
// Created by Plutex on 6/22/26.
//

#ifndef PLUENGINE_RENDERER_H
#define PLUENGINE_RENDERER_H

#include "GLFrameBuffer.h"
#include "PluEngine/Core.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/Objects/EngineObject.h"

namespace Plu
{
    class ShaderProgram;
    struct RenderSnapshot;
    struct ShadowCascadeData;

    //This is a render thread only object
    class Renderer
    {
        // Liczba kaskad cieni (Cascaded Shadow Maps) dla światła kierunkowego.
        static constexpr int kCascadeCount = 4;

        ApplicationInfo* mApplicationInfo;
        TOwningPointer<FrameBuffer> mMainBuffer;
        // Mapy głębi per-kaskada (DepthOnly). Tworzone eager w Initialize (wątek renderu
        // posiada kontekst GL), więc na ścieżce klatki nie ma per-klatkowego CreateObject.
        DynamicArray<TOwningPointer<FrameBuffer>> mCascadeFrameBuffers;

        // Pass 1: renderuje głębię casterów do map kaskad i zwraca macierze/splity kaskad.
        DynamicArray<ShadowCascadeData> RenderShadowPass(RenderSnapshot* snapshot, const Matrix4& cameraView);
    public:
        Renderer() = default;
        ~Renderer() = default;

        void Initialize(ApplicationInfo* applicationInfo);
        void RenderSnapshot(RenderSnapshot* snapshot);
        void Shutdown();
    };
}

#endif //PLUENGINE_RENDERER_H
