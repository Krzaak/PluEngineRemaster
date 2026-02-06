//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_RENDERER_H
#define PLUENGINE_RENDERER_H
#include "PluEngine/Objects/EngineObject.h"
#include "PluEngine/Core.h"
#include "PluSTL_FWD.h"
#include "FrameBuffer.h"
#include "Renderer.generated.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
    class IRenderable;
    class FrameBuffer;
    class Application;
    class IWindow;

    enum class LinuxWindowType
    {
        Unknown,
        SDL2,
        GLFW
    };

    inline const char *ToString(LinuxWindowType e)
    {
        switch (e) {
            case LinuxWindowType::Unknown: return "Unknown";
            case LinuxWindowType::SDL2: return "SDL2";
            case LinuxWindowType::GLFW: return "GLFW";
            default: return "unknown";
        }
    }

    PLU_CLASS()
    class PLU_API Renderer : public EngineObject
    {
        REFLECTION_BODY_RENDERER()
    private:
        LinuxWindowType WindowProvider = LinuxWindowType::Unknown;
        //ONLY PLACE WHERE THERE'S PTR TO ENTIRE APP!
        Application* mApplication;

        TOwningPointer<FrameBuffer> mMainBuffer;
        void RenderImGui();
        void RenderGame();
        DynamicArray<IRenderable*> mRenderables;
        DynamicArray<EngineObjectHandle> mRenderablesHandles;
    public:
        Renderer();
        void Init(Application* application);
        ~Renderer() override;

        TUsePointer<FrameBuffer> GetMainBuffer();
        void AddRenderable(IRenderable* renderable);

        Matrix4 GetProjectionMatrix();
        Matrix4 GetViewMatrix();

        void Init(const TUsePointer<IWindow>& appWindow);
        void OnUpdate(float deltaTime);
        void OnShutdown();
    };
}

#endif //PLUENGINE_RENDERER_H