#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "PluEngine/Objects/EngineObject.h"
#include "PluSTL_FWD.h"
#include "IEditorViewport.generated.h"

namespace Plu
{
    struct AssetDescriptor;
    struct IAssetData;
    struct EditorAppContext;
    class IEditorPanel;
    class IEditorAssetObject;
    //READ
    //In editor we have viewports which is for an asset, in the simplest form viewport is an empty dockspace
    //EditorPanels give each viewport functionality, they're "sections" of a viewport
    //Panels are not necessary, we can still have functionality without panels
    PLU_CLASS(Abstract)
    class IEditorViewport : public EngineObject
    {
        REFLECTION_BODY_IEDITORVIEWPORT()
    private:
        TUsePointer<AssetDescriptor> mAsset;
        bool mCanClose = true;
        bool mIsOpen = true;
        bool mCanBeSaved = false;
        GameHashMap<String, TOwningPointer<IEditorPanel>> mEditorPanels;
        ImGuiWindowClass* windowClass;
        ImGuiID dockID;
        DynamicArray<TUsePointer<IEditorPanel>> mPanelsToRegister;
        bool mWasSavedThisFrame = false;
    protected:
        EditorAppContext* mEditorAppContext;
        TUsePointer<class EngineObjectManager> mEngineObjectManager;
    public:
        IEditorViewport();
        void Initialize(const TUsePointer<AssetDescriptor> &assetObject);
        virtual ~IEditorViewport() override;
        void Shutdown();

        TUsePointer<AssetDescriptor> GetAssetDescriptor();
        virtual String GetWindowTitle(); //Imgui Window Title, with formating ID
        virtual String GetWindowName(); //Asset Name
        virtual String GetDockspaceName();

        void SetCanClose(bool canClose);
        [[nodiscard]] bool IsOpen() const;
        [[nodiscard]] bool IsCanBeSaved() const;
        bool WasSavedThisFrame() const;
        void MarkThisFrameAsSaved();
        void ViewportChangedAsset() const;

        TUsePointer<IEditorPanel> AddPanel(TypeInfo* classToCreate, bool canBeClosed);

        template<class T>
        T* GetPanelSlow(); //Get panel by type. SLOW!!!! Only use once when setting up docking

        virtual void OnOpened() = 0;
        virtual void OnClosed() = 0;
        virtual void OnPanelRegister() = 0;
        virtual void OnUpdate(float deltaTime) = 0;
        virtual void OnInit() {}
        
        [[nodiscard]] ImGuiWindowClass* GetViewportWindowClass() const;
    protected:
        void UpdatePanels(float deltaTime);
        bool BeginWindow();
        void EndWindow();
        [[nodiscard]] ImGuiID GetWindowDockID() const;
        void SetCanBeSaved(bool canBeSaved);
    };

    template <class T>
    T* IEditorViewport::GetPanelSlow()
    {
        for (const auto& panel : mEditorPanels)
        {
            if (T* result = dynamic_cast<T*>(panel.second.GetRaw()))
            {
                return result;
            }
        }
        return nullptr;
    }
}
