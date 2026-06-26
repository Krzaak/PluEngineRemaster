//
// Created by Plutex on 31.12.2025.
//

#ifndef PLUENGINE_EDITORAPP_H
#define PLUENGINE_EDITORAPP_H
#include "imgui.h"
#include "PluEngine/Application.h"

namespace Plu
{
    struct EditorAppContext;
    class EditorProjectManager;
    class EditorPanelManager;

    class PluEditor final : public Application
    {
        EditorAppContext* mEditorAppContext;
        TOwningPointer<EditorPanelManager> mPanelManager;
        TOwningPointer<EditorProjectManager> mEditorProjectManager;
        //This for passa on GH
        friend inline float DrawToolbarWindow(float toolbarHeight, int windowID);
    public:
        PluEditor();
        virtual ~PluEditor() override;

        bool OnInit() override;
        void OnPostInit() override;
        void OnShutdown() override;
        void OnTick(float deltaTime) override;

        void OnRequestedExit() override;

    private:
        // Builds the whole editor UI. Driven by the editor itself from OnTick(), bracketed by
        // ImGui::NewFrame()/Render(), and the resulting draw data is handed to the render
        // thread via RenderingManager::SubmitImGuiDrawData().
        void OnImGuiRender();
        void OnImGuiRenderEX(UInt64 windowID);
    };
}

#endif //PLUENGINE_EDITORAPP_H