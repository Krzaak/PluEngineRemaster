#include "EditorViewportManager.h"

#include <imgui_internal.h>

#include "EditorAppContext.h"
#include "EditorWindows/EditorWindowsManager.h"
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
    delete mWindowClass;
}

Plu::TUsePointer<Plu::IEditorPanel> Plu::EditorViewportManager::GetHoveredPanel()
{
    return mHoveredPanel;
}

void Plu::EditorViewportManager::SetCameraOwner(const TUsePointer<IEditorViewport>& viewport)
{
    // Raw compares: TUsePointer::operator== throws on a dead object, and either side can be one.
    IEditorViewport* incoming = viewport.IsValid() ? viewport.GetRaw() : nullptr;
    IEditorViewport* current = mCameraOwner.IsValid() ? mCameraOwner.GetRaw() : nullptr;
    if (current == incoming) return;

    if (current) current->SaveCameraState();
    mCameraOwner = viewport;
    if (incoming) incoming->ApplyCameraState();
}

Plu::TUsePointer<Plu::IEditorViewport> Plu::EditorViewportManager::GetCameraOwner() const
{
    return mCameraOwner;
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
    {
        viewport->Initialize(asset);
    }
    // Opens in the window the user is working in; an already open viewport is left where it is and
    // only brought to front (see the early returns above) — yanking it out of the window the user
    // put it in would be worse than making them switch windows.
    UInt32 targetWindow = 0;
    gEditorAppContext->EditorWindowsManager->TryGetActiveWindowID(targetWindow);
    viewport->SetWindowIDToRender(targetWindow);
    mViewports.PushBack(viewport);
    {
        viewport->OnOpened();
    }
    mWindowsToDock.PushBack({viewport->GetWindowTitle(), viewport->GetWindowIDToRender()});
}

void Plu::Register_Reflection_EditorViewportManager() {
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

void Plu::EditorViewportManager::Tick(float deltaTime, UInt32 windowID)
{
    for (const TOwningPointer<IEditorViewport>& viewport : mViewports)
    {
        if (viewport->GetWindowIDToRender() != windowID) continue;
        PLU_PROFILE_SCOPE(String::Format("Editor Viewport Build {0}", viewport->GetWindowName()));
        viewport->OnUpdate(deltaTime);
    }
    // Destruction is not per window: it must run exactly once per frame, so the main window owns it.
    if (windowID != 0) return;
    for (const TUsePointer<IEditorViewport> &traitor: mViewportsToAnnihilateFromExistanceInOurWorld) {
        // Drop the camera before the owner dies — a closing viewport has no view worth stashing,
        // and the next viewport to show up claims it.
        if (mCameraOwner.IsValid() && mCameraOwner.GetRaw() == traitor.GetRaw()) mCameraOwner = nullptr;
        traitor->OnClosed();
        traitor->Shutdown();
        mViewports.Remove(gEngineObjectManager->GetObjectAsOwner<IEditorViewport>(*traitor->GetEngineObjectHandle()));
        gEngineObjectManager->DestroyObject(*traitor->GetEngineObjectHandle());
    }
    mViewportsToAnnihilateFromExistanceInOurWorld.Clear();
}

void Plu::EditorViewportManager::ReturnViewportsFromWindow(UInt32 windowID)
{
    if (windowID == 0) return;
    for (const TOwningPointer<IEditorViewport>& viewport : mViewports)
    {
        if (viewport->GetWindowIDToRender() != windowID) continue;
        viewport->SetWindowIDToRender(0);
        // Re-dock it in the main window: its dock node lived in the window that just died.
        mWindowsToDock.PushBack({viewport->GetWindowTitle(), viewport->GetWindowIDToRender()});
    }
}

void Plu::EditorViewportManager::ReturnViewportPanelsFromWindow(UInt32 windowID)
{
    if (windowID == 0) return;
    for (const TOwningPointer<IEditorViewport>& viewport : mViewports)
    {
        // Back to wherever the viewport itself lives now — which is what "the panel goes home"
        // means; it is drawn again by IEditorViewport::UpdatePanels from that point on.
        viewport->MovePanelsFollowingViewport(windowID, viewport->GetWindowIDToRender());
    }
}

void Plu::EditorViewportManager::MoveViewportToWindow(EngineObjectHandle viewport, UInt32 targetWindowID)
{
    TUsePointer<IEditorViewport> viewportPtr = gEngineObjectManager->GetObjectAsUser<IEditorViewport>(viewport);
    if (!viewportPtr) return;
    if (viewportPtr->GetWindowIDToRender() == targetWindowID) return;

    const UInt32 previousWindowID = viewportPtr->GetWindowIDToRender();
    viewportPtr->SetWindowIDToRender(targetWindowID);
    // Panels that were following the viewport keep following it; one that had been pulled out into
    // its own SinglePanel window stays where it is.
    viewportPtr->MovePanelsFollowingViewport(previousWindowID, targetWindowID);
    // Its dock node belongs to the window it is leaving, so it needs a fresh one over there.
    mWindowsToDock.PushBack({viewportPtr->GetWindowTitle(), targetWindowID});
}

void Plu::EditorViewportManager::DockNewViewports(UInt32 windowID)
{
    if (mWindowsToDock.IsEmpty()) return;
    // Docking runs against the *current* ImGui context, so a viewport can only be docked while its
    // own window's frame is being built; entries for other windows wait for their turn.
    const EditorWindowInfo* windowInfo = gEditorAppContext->EditorWindowsManager->GetWindowInfo(windowID);
    if (!windowInfo) return;

    DynamicArray<PendingViewportDock> stillToDock;
    bool dockedAny = false;
    for (const PendingViewportDock& pending : mWindowsToDock)
    {
        if (pending.WindowID != windowID) {
            stillToDock.PushBack(pending);
            continue;
        }
        ImGui::DockBuilderDockWindow(pending.WindowTitle.CStr(), windowInfo->DockspaceId);
        PLU_CORE_INFO("New Viewport Opened, Name {}", pending.WindowTitle.CStr());
        ImGui::SetWindowFocus(pending.WindowTitle.CStr());
        dockedAny = true;
    }
    if (dockedAny) ImGui::DockBuilderFinish(windowInfo->DockspaceId);
    mWindowsToDock = stillToDock;
}

void Plu::EditorViewportManager::Shutdown()
{
    mCameraOwner = nullptr;
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
