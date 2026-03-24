//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_RENDERER_H
#define PLUENGINE_RENDERER_H
#include "PluEngine/Objects/EngineObject.h"
#include "PluEngine/Core.h"
#include "PluSTL_FWD.h"
#include "GLFrameBuffer.h"
#include "Renderer.generated.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
    class IRendererCamera;
    class JoltWireframeRenderer;
    class JoltPointRenderer;
    class CameraComponent;
    class IRenderable;
    class FrameBuffer;
    class Application;
    class IWindow;

    PLU_ENUM(PyNamespace=Plu)
    enum class PhysicsDebugRender
    {
        NONE,
        POINTS,
        WIREFRAME,
        BOTH
    };

    PLU_CLASS()
    class PLU_API Renderer : public EngineObject
    {
        REFLECTION_BODY_RENDERER()
    private:
        //ONLY PLACE WHERE THERE'S PTR TO ENTIRE APP!
        Application* mApplication;

        TOwningPointer<FrameBuffer> mMainBuffer;
        void RenderImGui(int windowID);
        void RenderGame(float deltaTime);
        DynamicArray<IRenderable*> mRenderables;
        IRendererCamera* mActiveCamera = nullptr;

        TOwningPointer<JoltPointRenderer> mPointRenderer;
        TOwningPointer<JoltWireframeRenderer> mWireframeRenderer;
    public:
        Renderer();
        void Init(Application* application);
        ~Renderer() override;

        TUsePointer<FrameBuffer> GetMainBuffer();
        void AddRenderable(IRenderable* renderable);
        void RemoveRenderable(IRenderable* renderable);
        void ClearRenderables();

        void SetCamera(IRendererCamera* newCamera);
        IRendererCamera* GetCamera();

        PhysicsDebugRender PhysicsDebugRenderMode = PhysicsDebugRender::WIREFRAME;
        Vec3 PhysicsDebugRenderColorPoints = Vec3(1,0,0);
        Vec3 PhysicsDebugRenderColorWireframe = Vec3(1,0,0);

        [[nodiscard]] Matrix4 GetProjectionMatrix() const;
        [[nodiscard]] Matrix4 GetViewMatrix();

        void Init(const TUsePointer<IWindow>& appWindow);
        void OnUpdate(float deltaTime);
        void OnShutdown();
    };
}

#endif //PLUENGINE_RENDERER_H