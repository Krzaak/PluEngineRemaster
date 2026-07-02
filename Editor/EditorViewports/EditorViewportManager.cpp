#include "EditorViewportManager.h"

#include <imgui_internal.h>

#include "EditorAppContext.h"
#include "EditorInterface.h"
#include "IEditorViewport.h"
#include "DefinedViewports/Scene/SceneViewport.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::EditorViewportManager::EditorViewportManager()
{
    mWindowClass = new ImGuiWindowClass();
    mWindowClass->DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoSplit | ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_NoWindowMenuButton;
}

Plu::EditorViewportManager::~EditorViewportManager()
{
}

Plu::TUsePointer<Plu::IEditorPanel> Plu::EditorViewportManager::GetHoveredPanel()
{
    return mHoveredPanel;
}

void Plu::EditorViewportManager::CloseViewport(EngineObjectHandle viewport)
{
    TUsePointer<IEditorViewport> viewportPtr = gEngineObjectManager->GetObjectAsUser<IEditorViewport>(viewport);
    if (!viewportPtr) return;
    mViewportsToAnnihilateFromExistanceInOurWorld.PushBack(viewportPtr);
}

void Plu::EditorViewportManager::CreateViewport(const PathW& assetPath, const TypeInfo* classOfViewport)
{
    if (!classOfViewport) return;
    if (classOfViewport == SceneViewport::GetStaticClass() && GetViewport(SceneViewport::GetStaticClass())) {
        TUsePointer<AssetDescriptor> asset = gEditorAppContext->EditorAssetManager->GetAssetDescriptor(assetPath.ToString().ToNarrow());
        GetViewport(SceneViewport::GetStaticClass())->Initialize(asset);
        GetViewport(SceneViewport::GetStaticClass())->mBringToFront = true;
        return;
    }
    TUsePointer<AssetDescriptor> asset = gEditorAppContext->EditorAssetManager->GetAssetDescriptor(assetPath.ToString().ToNarrow());
    if (GetViewportForAsset(asset))
    {
        GetViewportForAsset(asset)->mBringToFront = true;
        return;
    }
    TOwningPointer<IEditorViewport> viewport = gEngineObjectManager->GetObjectAsOwner<IEditorViewport>(gEngineObjectManager->CreateObject(classOfViewport)->GetObjectHandle());
    viewport->Initialize(asset);
    mViewports.PushBack(viewport);
    viewport->OnOpened();

    mWindowsToDock.PushBack(viewport->GetWindowTitle());
}

Plu::TUsePointer<Plu::IEditorViewport> Plu::EditorViewportManager::GetViewport(TypeInfo *viewportClass)
{
    for (auto viewport : mViewports) {
        if (viewport->GetClass() == viewportClass) return viewport;
    }
    return nullptr;
}

Plu::TUsePointer<Plu::IEditorViewport> Plu::EditorViewportManager::GetViewportForAsset(
    TUsePointer<AssetDescriptor> asset)
{
    for (auto viewport : mViewports)
    {
        if (viewport->GetAssetDescriptor() == asset) return viewport;
    }
    return nullptr;
}

bool Plu::EditorViewportManager::AreThereViewportsToDock() const
{
    return !mWindowsToDock.IsEmpty();
}

void Plu::EditorViewportManager::Tick(float deltaTime)
{
    for (const TOwningPointer<IEditorViewport>& viewport : mViewports)
    {
        viewport->OnUpdate(deltaTime);
    }
    for (const TUsePointer<IEditorViewport> &traitor: mViewportsToAnnihilateFromExistanceInOurWorld) {
        traitor->OnClosed();
        traitor->Shutdown();
        mViewports.Remove(gEngineObjectManager->GetObjectAsOwner<IEditorViewport>(*traitor->GetEngineObjectHandle()));
        gEngineObjectManager->DestroyObject(*traitor->GetEngineObjectHandle());
    }
    mViewportsToAnnihilateFromExistanceInOurWorld.Clear();
}

void Plu::EditorViewportManager::DockNewViewports()
{
    if (!mWindowsToDock.IsEmpty())
    {
        for (const String& windowTitle : mWindowsToDock)
        {
            ImGui::DockBuilderDockWindow(windowTitle.CStr(), gDockspaceId);
            ImGui::SetWindowFocus(windowTitle.CStr());
        }
        ImGui::DockBuilderFinish(gDockspaceId);
        mWindowsToDock.Clear();
    }
}

void Plu::EditorViewportManager::Shutdown()
{
    for (const auto& viewport : mViewports)
    {
        viewport->OnClosed();
        viewport->Shutdown();
    }
    for (TOwningPointer<IEditorViewport> viewport : mViewports)
    {
        gEngineObjectManager->DestroyObject(*viewport->GetEngineObjectHandle());
        viewport = nullptr;
    }
    mViewports.Clear();
}

ImGuiWindowClass* Plu::EditorViewportManager::GetViewportWindowClass() const
{
    return mWindowClass;
}
