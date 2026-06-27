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

        // VAO/VBO debugowej geometrii fizyki (linie + punkty). Tworzone eager w Initialize na
        // wątku renderu; uploadowane per-klatkę z płaskich buforów snapshotu (pos(3)+color(3)).
        unsigned int mDebugVao = 0;
        unsigned int mDebugVbo = 0;

        // Pass 1: renderuje głębię casterów do map kaskad i zwraca macierze/splity kaskad.
        DynamicArray<ShadowCascadeData> RenderShadowPass(RenderSnapshot* snapshot, const Matrix4& cameraView);

        // Rysuje debugową geometrię fizyki ze snapshotu (linie + punkty) shaderem DebugLine.
        void RenderDebugGeometry(RenderSnapshot* snapshot, const Matrix4& viewProj);
    public:
        Renderer() = default;
        ~Renderer() = default;

        TUsePointer<FrameBuffer> GetMainFrameBuffer();

        void Initialize(ApplicationInfo* applicationInfo);
        void RenderSnapshot(RenderSnapshot* snapshot);
        void Shutdown();
    };
}

#endif //PLUENGINE_RENDERER_H
