#include "IEditorViewport.h"

#include <imgui_internal.h>

#include "EditorAppContext.h"
#include "EditorViewportManager.h"
#include "IEditorPanel.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::IEditorViewport::IEditorViewport()
{
    windowClass = new ImGuiWindowClass();
    mEditorAppContext = gEditorAppContext;
    mEngineObjectManager = gEngineObjectManager;
}

void Plu::IEditorViewport::Initialize(const TUsePointer<AssetDescriptor> &assetObject)
{
    mAsset = assetObject;
    mCanBeSaved = assetObject->LoaderType == AssetLoaderType::JSON;
    OnInit();
}

Plu::IEditorViewport::~IEditorViewport()
{
    delete windowClass;
}

void Plu::IEditorViewport::Shutdown()
{
    for (const std::pair<String, TOwningPointer<IEditorPanel>> panel : mEditorPanels)
    {
        panel.second->OnClosed();
        gEngineObjectManager->DestroyObject(*panel.second->GetEngineObjectHandle());
    }
    mEditorPanels.Clear();
}

Plu::TUsePointer<Plu::AssetDescriptor> Plu::IEditorViewport::GetAssetDescriptor()
{
    return mAsset;
}

Plu::String Plu::IEditorViewport::GetWindowTitle()
{
    return GetWindowName();
}

Plu::String Plu::IEditorViewport::GetWindowName()
{
    return mAsset ? mAsset->AssetName : "NOASSET";
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

bool Plu::IEditorViewport::IsCanBeSaved() const
{
    return mCanBeSaved;
}

bool Plu::IEditorViewport::WasSavedThisFrame() const
{
    return mWasSavedThisFrame;
}

void Plu::IEditorViewport::MarkThisFrameAsSaved()
{
    mWasSavedThisFrame = true;
}

void Plu::IEditorViewport::ViewportChangedAsset() const
{
    gEditorAppContext->EditorAssetManager->MarkAssetDirty(mAsset);
}

Plu::TUsePointer<Plu::IEditorPanel> Plu::IEditorViewport::AddPanel(TypeInfo *classToCreate, bool canBeClosed)
{
    TUsePointer<IEditorPanel> newPanel = DynamicCast<IEditorPanel>(gEngineObjectManager->CreateObject(classToCreate));
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
    if (mEditorAppContext->EditorAssetManager->IsAssetDirty(mAsset)) {
        flags |= ImGuiWindowFlags_UnsavedDocument;
    }
    static GameHashMap<String, bool> lastWindowState;

    if (mBringToFront)
    {
        ImGui::SetNextWindowFocus();
        mBringToFront = false;
    }

    bool open = ImGui::Begin(GetWindowTitle().CStr(), mCanClose ? &mIsOpen : nullptr, flags);
    if (open != lastWindowState[GetWindowTitle()]) {
        if (open) {
            for (auto panel : mEditorPanels) {
                panel.second->OnOpened();
            }
        } else {
            for (auto panel : mEditorPanels) {
                panel.second->OnClosed();
            }
        }
    }
    mWasSavedThisFrame = false;
    if (open)
    {
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
            }
            mPanelsToRegister.Clear();
            OnPanelRegister();
        }


        if (mCanBeSaved) {
            if (ImGui::Shortcut(ImGuiMod_Ctrl + ImGuiKey_S) && !WasSavedThisFrame()) {
                MarkThisFrameAsSaved();
                mEditorAppContext->EditorAssetManager->SaveAsset(mAsset);
            }
        }
    }
    lastWindowState[GetWindowTitle()] = open;
    if (!mIsOpen) {
        gEditorAppContext->EditorViewportManager->CloseViewport(*this->GetEngineObjectHandle());
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

void Plu::IEditorViewport::SetCanBeSaved(bool canBeSaved)
{
    mCanBeSaved = canBeSaved;
}
