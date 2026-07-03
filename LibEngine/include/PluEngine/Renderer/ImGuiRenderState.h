//
// Created by Plutex on 7/3/26.
//

#ifndef PLUENGINE_IMGUIRENDERSTATE_H
#define PLUENGINE_IMGUIRENDERSTATE_H

#include <PluSTL_FWD.h>
#include "PluEngine/Core.h"

struct ImGuiContext;

namespace Plu
{
    class IWindow;

    // Cykl życia kontekstu ImGui, rozdzielony między wątki tak jak backendy:
    // - CreateContext() na Main: kontekst + IO/DPI + styl silnika + backend platformowy
    //   (Win32/SDL3). Woła to RenderingManager::InitializeImGuiContext() zaraz po IWindow::Init().
    // - InitRendererBackend()/ShutdownRendererBackend() na render threadzie: backend OpenGL3
    //   (potrzebuje bieżącego kontekstu GL) — patrz RenderingManager::RenderThreadEnter/Exit.
    class PLU_API ImGuiRenderState
    {
    public:
        // Main thread. Tworzy i konfiguruje kontekst ImGui dla okna; zwraca go, żeby okno
        // mogło go przechować (WndProc/eventy SDL czytają go przez IWindow::GetImGuiContext()).
        ImGuiContext* CreateContext(TUsePointer<IWindow> window);

        // Render thread, z bieżącym kontekstem GL.
        void InitRendererBackend(ImGuiContext* context);
        void ShutdownRendererBackend();

        [[nodiscard]] ImGuiContext* GetContext() const { return mContext; }

    private:
        // Styl silnika: zaokrąglenia, paddingi i czarno-szara paleta z granatowym akcentem.
        void ApplyEngineStyle(TUsePointer<IWindow> window);

        ImGuiContext* mContext = nullptr;
        bool mRendererBackendInitialized = false;
    };
}

#endif //PLUENGINE_IMGUIRENDERSTATE_H
