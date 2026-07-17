//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_WINDOW_H
#define PLUENGINE_WINDOW_H

#include <PluSTL_FWD.h>
#include "Array/Array.h"
#include "PluEngine/Objects/EngineObject.h"
#include "PluEngine/Core.h"
#include "Window.generated.h"
#include "PluEngine/Application.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
    PLU_ENUM(PyExport, PyNamespace=Plu)
    enum class FullscreenType
    {
        // Zwykłe okno z ramką (albo bez, jeśli WindowProperties::Borderless) i przywróconą pozycją.
        Windowed,
        // Wyłączny fullscreen: zmienia tryb wideo monitora na żądaną rozdzielczość.
        Fullscreen,
        // Bezramkowe okno rozciągnięte na cały monitor, bez zmiany trybu wideo.
        BorderlessWindow
    };

    // Tryb wideo monitora. RefreshRate w Hz; 0 = nieznany.
    struct DisplayMode
    {
        int Width = 0;
        int Height = 0;
        float RefreshRate = 0.0f;
    };

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
        ImGuiContext* mImGuiContext = nullptr;

        friend void Application::DispatchWindowClose(TUsePointer<IWindow> window);
        virtual void Close() = 0;
    public:
        explicit IWindow() = default;
        virtual ~IWindow() = default;

        void SetWindowProperties(const WindowProperties& properties);

        virtual void Init() = 0;
        virtual void OnUpdate(float deltaTime) = 0;
        virtual void Shutdown() = 0;

        virtual bool IsRunning() = 0;

        virtual int GetWidth() = 0;
        virtual int GetHeight() = 0;

        virtual void Minimize() = 0;
        virtual void Maximize() = 0;
        virtual bool IsWindowMinimized() = 0;
        virtual bool IsWindowMaximized() = 0;

        // Przełącza tryb wyświetlania. `resolution` dotyczy tylko FullscreenType::Fullscreen —
        // {0,0} oznacza natywną rozdzielczość pulpitu. Windowed przywraca zapamiętaną pozycję
        // i rozmiar sprzed pierwszego przejścia w fullscreen. Wołać z main threadu.
        virtual void MakeFullscreen(FullscreenType type, IVec2 resolution = IVec2(0, 0)) = 0;
        virtual FullscreenType GetFullscreenType() const = 0;
        // Tryby wideo monitora, na którym stoi okno, posortowane rosnąco (szerokość, wysokość, Hz).
        virtual DynamicArray<DisplayMode> GetSupportedDisplayModes() = 0;
        // Natywna rozdzielczość (tryb pulpitu) monitora, na którym stoi okno.
        virtual IVec2 GetDesktopResolution() = 0;

        virtual bool HasWindowFocus() = 0;

        virtual bool IsVSyncEnabled() = 0;
        virtual void SetVSyncEnabled(bool enabled) = 0;

        virtual void* GetWindowHandle() = 0;
        virtual void* GetGLContext() = 0;

        virtual void MakeGLContextCurrent() = 0;
        // Odpina kontekst GL od bieżącego wątku, aby inny wątek (render thread) mógł go przejąć.
        virtual void ReleaseGLContext() = 0;
        virtual void SwapBuffer() = 0;

        virtual void SetWindowTitle(String title) = 0;

        static Plu::TOwningPointer<IWindow> PlutexCreateWindow(const WindowProperties& properties, const TUsePointer<EngineObjectManager>& objectManager, ApplicationInfo *
                                                               applicationInfo);

        virtual void SetCursorVisibility(bool visible) = 0;
        virtual void SetCursorPosition(IVec2 pos) = 0;
        virtual IVec2 GetCursorPosition() = 0;

        // Kontekst ImGui tworzy i konfiguruje RenderingManager::InitializeImGuiContext()
        // (przez ImGuiRenderState); okno tylko go przechowuje dla WndProc/eventów.
        void SetImGuiContext(ImGuiContext* context) { mImGuiContext = context; }
        ImGuiContext* GetImGuiContext() const { return mImGuiContext; }

        bool ImGuiItemHovered = false;
        bool UpdateImGui = true;
    };
}

#endif //PLUENGINE_WINDOW_H