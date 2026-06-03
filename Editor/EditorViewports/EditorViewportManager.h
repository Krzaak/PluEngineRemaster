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
        DynamicArray<TOwningPointer<IEditorViewport>> mViewports; //Viewport is for a given asset

        ImGuiWindowClass* mWindowClass;
        DynamicArray<String> mWindowsToDock;

        TUsePointer<IEditorPanel> mHoveredPanel;

        DynamicArray<TUsePointer<IEditorViewport>> mViewportsToAnnihilateFromExistanceInOurWorld;
    public:
        EditorViewportManager();
        ~EditorViewportManager() override;

        TUsePointer<IEditorPanel> GetHoveredPanel();

        void CloseViewport(EngineObjectHandle viewport);

        void CreateViewport(const PathW& assetPath, const TypeInfo* classOfViewport);
        TUsePointer<IEditorViewport> GetViewport(TypeInfo* viewportClass);

        bool AreThereViewportsToDock() const;

        void Tick(float deltaTime);
        void DockNewViewports();
        void Shutdown();

        ImGuiWindowClass* GetViewportWindowClass() const;
    };
}
