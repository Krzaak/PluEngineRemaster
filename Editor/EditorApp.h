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
        friend inline float DrawToolbarWindow(float toolbarHeight);
    public:
        PluEditor();
        virtual ~PluEditor() override;

        void OnInit() override;
        void OnPostInit() override;
        void OnShutdown() override;
        void OnImGuiRender() override;
        void OnImGuiRenderEX(UInt64 windowID) override;
        void OnTick(float deltaTime) override;
    };
}

#endif //PLUENGINE_EDITORAPP_H