#include "IEditorViewport.h"

#include <imgui/imgui_internal.h>

#include "EditorAppContext.h"
#include "EditorViewportManager.h"
#include "IEditorPanel.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/Objects/EngineObjectManager.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::IEditorViewport::IEditorViewport()
{
    windowClass = new ImGuiWindowClass();
}

void Plu::IEditorViewport::Initialize(const TUsePointer<IEditorAssetObject> &assetObject)
{
    mAsset = assetObject;
}

Plu::IEditorViewport::~IEditorViewport()
{

    for (const std::pair<String, TOwningPointer<IEditorPanel>> panel : mEditorPanels)
    {
        gEngineObjectManager->DestroyObject(*panel.second->GetEngineObjectHandle());
    }
    mEditorPanels.Clear();
    //TODO
    // mpEditorState->ViewportManager->SetHoveredPanel(nullptr);
}

Plu::TUsePointer<Plu::IEditorAssetObject> Plu::IEditorViewport::GetAssetObject()
{
    return mAsset;
}

Plu::String Plu::IEditorViewport::GetWindowTitle()
{
    return GetWindowName();
}

Plu::String Plu::IEditorViewport::GetWindowName()
{
    return mAsset ? mAsset->GetAssetName() : "NOASSET";
}

Plu::String Plu::IEditorViewport::GetDockspaceName()
{
    return GetWindowName() + "Dockspace";
}

void Plu::IEditorViewport::SetCanClose(bool canClose)
{
    mCanClose = canClose;
}

bool Plu::IEditorViewport::IsOpen() const
{
    return mIsOpen;
}

Plu::TUsePointer<Plu::IEditorPanel> Plu::IEditorViewport::AddPanel(TypeInfo *classToCreate, bool canBeClosed)
{
    TOwningPointer<IEditorPanel> newPanel = DynamicCast<IEditorPanel>(gEngineObjectManager->CreateObject(classToCreate));
    mPanelsToRegister.PushBack(newPanel);
    newPanel->Initialize(gEngineObjectManager->GetObjectAsUser<IEditorViewport>(*this->GetEngineObjectHandle()), false);
    return newPanel;
}

ImGuiWindowClass* Plu::IEditorViewport::GetViewportWindowClass() const
{
    return windowClass;
}

void Plu::IEditorViewport::UpdatePanels(float deltaTime)
{
    for (std::pair<String, TOwningPointer<IEditorPanel>> panel : mEditorPanels)
    {
        panel.second->OnUpdate(deltaTime);
    }
}

bool Plu::IEditorViewport::BeginWindow()
{
    if (!ImGui::GetWindowDockNode()) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    }
    ImGui::SetNextWindowClass(gEditorAppContext->EditorViewportManager->GetViewportWindowClass());
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
    bool open = ImGui::Begin(GetWindowTitle().CStr(), mCanClose ? &mIsOpen : nullptr, flags);
    if (open)
    {
        //TODO
        //mpEditorState->SelectedAsset = mAsset.GetRaw();
        windowClass->ClassId = ImGui::GetID(GetDockspaceName().CStr());
        dockID = ImGui::GetID(GetDockspaceName().CStr());
        ImGui::DockSpace(dockID,ImVec2(0,0),0,windowClass);

        //Now register all unregistered panels
        
        if (!mPanelsToRegister.IsEmpty())
        {
            for (const auto& panel : mPanelsToRegister)
            {
                mEditorPanels.Insert(panel->GetPanelName(), gEngineObjectManager->GetObjectAsOwner<IEditorPanel>(*panel->GetEngineObjectHandle()));
                //ImGui::DockBuilderDockWindow(panel->GetPanelTitle().c_str(), dockID);
                //ImGui::DockBuilderFinish(dockID);
                panel->OnOpened();
                PLU_INFO("Registered panel {}", panel->GetPanelName().CStr());
            }
            mPanelsToRegister.Clear();
            OnPanelRegister();
            PLU_INFO("Register Panels, complete for Viewport {}", GetWindowTitle().CStr());
        }
    }
    return open;
}

void Plu::IEditorViewport::EndWindow()
{
    ImGui::End();
    if (!ImGui::GetWindowDockNode()) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }    
}

ImGuiID Plu::IEditorViewport::GetWindowDockID() const
{
    return dockID;
}
