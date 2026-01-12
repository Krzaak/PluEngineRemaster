#pragma once

#include <filesystem>


#include <imgui/imgui.h>

#include "IEditorViewport.h"
#include "PluSTL_FWD.h"
#include "PluEngine/Objects/EngineObject.h"
#include "EditorViewportManager.generated.h"

namespace Plu
{
    class IEditorPanel;
    class EngineObjectManager;
    //Viewports are newer and better solution but not all cases require them.
    //So general thought is that viewports are for assets and panels for everything else e.g. AssetBrowser, Theme Editor
    PLU_CLASS()
    class EditorViewportManager : public EngineObject 
    {
        REFLECTION_BODY_EDITORVIEWPORTMANAGER()
    private:
        DynamicArray<TOwningPointer<IEditorViewport>> viewports; //Viewport is for a given asset

        ImGuiWindowClass* window_class;
        DynamicArray<String> mWindowsToDock;
        ImGuiID dockspace_id;

        TUsePointer<IEditorPanel> mHoveredPanel;
    public:
        EditorViewportManager();
        ~EditorViewportManager() override;

        TUsePointer<IEditorPanel> GetHoveredPanel();

        void CreateViewport(const PathW& assetPath, const TypeInfo* classOfViewport);

        void Tick(float deltaTime);
        void Shutdown();

        ImGuiWindowClass* GetViewportWindowClass() const;
    };
}
