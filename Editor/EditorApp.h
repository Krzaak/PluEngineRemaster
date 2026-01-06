//
// Created by Plutex on 31.12.2025.
//

#ifndef PLUENGINE_EDITORAPP_H
#define PLUENGINE_EDITORAPP_H
#include "imgui.h"
#include "PluEngine/Application.h"

namespace Plu
{
    class EditorPanelManager;

    class PluEditor final : public Application
    {
        TOwningPointer<EditorPanelManager> mPanelManager;
        ImGuiWindowClass* mWindowClass;
        ImGuiID mDockspaceId;
    public:
        PluEditor();
        virtual ~PluEditor() override;

        void OnInit() override;
        void OnPostInit() override;
        void OnShutdown() override;
        void OnImGuiRender() override;

        void DrawMainEngineWindow();
        float DrawToolbarWindow(float toolbarHeight);
    };
}

#endif //PLUENGINE_EDITORAPP_H