//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_WINDOW_H
#define PLUENGINE_WINDOW_H

#include <PluSTL_FWD.h>
#include "PluEngine/Objects/EngineObject.h"
#include "PluEngine/Core.h"
#include "Window.generated.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
    struct WindowProperties
    {
        String Title;
        int Width;
        int Height;
        bool InitImGui;
        bool Borderless;

        WindowProperties() : Title("New Window"), Width(1000), Height(720), InitImGui(false), Borderless(false) {}
        WindowProperties(const String &title) : WindowProperties()
        {
            Title = title;
        }
    };

    class EngineObjectManager;

    PLU_CLASS(Abstract)
    class PLU_API IWindow : public EngineObject
    {
        REFLECTION_BODY_IWINDOW()
    protected:
        WindowProperties mProperties;
        ApplicationInfo* mApplicationInfo;
        ImGuiContext* mImGuiContext;
    public:
        explicit IWindow() = default;
        virtual ~IWindow() = default;

        void SetWindowProperties(const WindowProperties& properties);

        virtual void Init() = 0;
        virtual void OnUpdate(float deltaTime) = 0;
        virtual void Shutdown() = 0;

        virtual bool IsRunning() = 0;
        virtual void Close() = 0;

        virtual int GetWidth() = 0;
        virtual int GetHeight() = 0;

        virtual void Minimize() = 0;
        virtual void Maximize() = 0;
        virtual bool IsWindowMinimized() = 0;
        virtual bool IsWindowMaximized() = 0;

        virtual bool IsVSyncEnabled() = 0;
        virtual void SetVSyncEnabled(bool enabled) = 0;

        virtual void* GetWindowHandle() = 0;
        virtual void* GetGLContext() = 0;

        virtual void MakeGLContextCurrent() = 0;
        virtual void SwapBuffer() = 0;

        virtual void SetWindowTitle(String title) = 0;

        static Plu::TOwningPointer<IWindow> PlutexCreateWindow(const WindowProperties& properties, const TUsePointer<EngineObjectManager>& objectManager, ApplicationInfo *
                                                               applicationInfo);

        static void SetCursorVisibility(bool visible);
        virtual void SetCursorPosition(IVec2 pos) = 0;
        virtual IVec2 GetCursorPosition() = 0;

        void CreateImGuiContext();
        ImGuiContext* GetImGuiContext() const { return mImGuiContext; }

        bool ImGuiItemHovered = false;
        bool UpdateImGui = true;
    };
}

#endif //PLUENGINE_WINDOW_H