//
// Created by Plutex on 31.12.2025.
//

#ifndef PLUENGINE_EDITORAPP_H
#define PLUENGINE_EDITORAPP_H
#include "imgui.h"
#include "PluEngine/Application.h"

namespace Plu
{
    class EditorAssetImporter;
    struct EditorAppContext;
    class EditorProjectManager;
    class EditorPanelManager;

    class PluEditor final : public Application
    {
        EditorAppContext* mEditorAppContext;
        TOwningPointer<EditorPanelManager> mPanelManager;
        TOwningPointer<EditorProjectManager> mEditorProjectManager;
        // True while a font-atlas rebuild (e.g. font-size change) is still settling and the ImGui
        // handoff must stay in lockstep with the render thread. See OnTick() and
        // RenderingManager::BeginImGuiLockstep().
        bool mImGuiAtlasSettling = false;
        //This for passa on GH
        friend inline float DrawToolbarWindow(float toolbarHeight, int windowID);

        bool mAssetSaveConfirmShow = false;
        bool mAssetSaveConfirm = false;

        bool mIsDropOnWindow = false;
        TUsePointer<EditorAssetImporter> mAssetImporter;
        DynamicArray<Path> mPathsToImport;
        void ClearAfterImport();
    public:
        PluEditor();
        virtual ~PluEditor() override;

        bool OnInit() override;
        void OnPostInit() override;
        void OnShutdown() override;
        void OnTick(float deltaTime) override;

        void OnRequestedGameExit() override;

        void OnRequestedWindowClose(TUsePointer<IWindow> window) override;

    private:
        // Builds the whole editor UI. Driven by the editor itself from OnTick(), bracketed by
        // ImGui::NewFrame()/Render(), and the resulting draw data is handed to the render
        // thread via RenderingManager::SubmitImGuiDrawData().
        void OnImGuiRender();
        void OnImGuiRenderEX(UInt64 windowID);
        void DrawNewProjectPopup();
    };

    void LoadFactories();
}

#endif //PLUENGINE_EDITORAPP_H