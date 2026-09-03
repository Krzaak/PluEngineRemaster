#include "IEditorPanel.h"

#include <imgui_internal.h>

#include <utility>

#include "EditorAppContext.h"
#include "EditorViewportManager.h"
#include "EditorWindows/EditorWindowMoveMenu.h"
#include "EditorWindows/EditorWindowsManager.h"
#include "PluEngine/Core/Objects/EngineObjectManager.h"
#include "PluEngine/AssetCore/EngineAssetManager.h"

extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;

bool Plu::IEditorPanel::BeginPanel(ImGuiWindowFlags flags)
{
    // In a SinglePanel window the panel *is* the window: the OS title bar (drawn by
    // PluEditor::DrawSinglePanelWindow) already carries the name and the window controls, and the
    // geometry is the window's. Leaving the normal flags on gave a second title bar with the same
    // name, an ImGui resize grip inside the OS resize border, and a dock preview over a dockspace
    // that does not exist here.
    const EditorWindowInfo* hostWindow = gEditorAppContext->EditorWindowsManager->GetWindowInfo(mWindowIDToRender);
    const bool fillsOwnWindow = hostWindow && hostWindow->Kind == EEditorWindowKind::SinglePanel;
    if (fillsOwnWindow) {
        flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoBringToFrontOnFocus;
    }
    if (!fillsOwnWindow && !ImGui::GetWindowDockNode()) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    }
    mFillsOwnWindow = fillsOwnWindow;
    // The parent viewport's window class is tied to a dockspace living in the parent's ImGui
    // context. Once the panel has been pulled out into its own window that class means nothing
    // there (and would dock the panel into a node of a different context), so it only applies
    // while panel and viewport are in the same window.
    if (GetParentViewport() && GetParentViewport()->GetWindowIDToRender() == mWindowIDToRender) {
        ImGui::SetNextWindowClass(GetParentViewport()->GetViewportWindowClass());
    }
    // BringToFront is requested during the viewport's layout build, i.e. on the same frame the dock
    // node is (re)created — the tab bar doesn't exist yet, so a single SetNextWindowFocus is lost.
    // Keep forcing focus every frame until the node actually reports this window as its selected tab,
    // then stop so the user can switch tabs freely.
    if (mBringToFront) {
        ImGui::SetNextWindowFocus();
    }
    // No close box either — a SinglePanel window is closed with its own title bar's controls.
    bool open = ImGui::Begin(GetPanelTitle().CStr(), (mCanClose && !fillsOwnWindow) ? &mIsOpen : nullptr, flags);
    // Right-click on the panel's dock tab.
    if (ImGui::BeginPopupContextItem()) {
        DrawMoveToWindowMenu(mWindowIDToRender, EEditorWindowKind::SinglePanel, GetPanelTitle(),
            [this](UInt32 targetWindowID) {
                gEditorAppContext->EditorWindowsManager->MoveViewportPanelToWindow(
                    gEngineObjectManager->GetObjectAsUser<IEditorPanel>(*this->GetEngineObjectHandle()),
                    targetWindowID);
            });
        ImGui::EndPopup();
    }
    if (mBringToFront) {
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window && window->DockNode && window->DockNode->SelectedTabId == window->TabId) {
            mBringToFront = false;
        }
    }
    if (open) {
        // Camera navigation is hover-gated, so hovering a viewport's panel is exactly the moment
        // its camera should become the live one. Claiming here (before the panel body runs) means
        // the panel already sees its own restored view this frame. ChildWindows: the render image
        // usually sits inside a BeginChild.
        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows)) {
            mEditorViewport->ClaimEditorCamera();
        }
        if (mEditorViewport->IsCanBeSaved()) {
            if (ImGui::Shortcut(ImGuiMod_Ctrl + ImGuiKey_S) && !mEditorViewport->WasSavedThisFrame()) {
                mEditorViewport->MarkThisFrameAsSaved();
                gEditorAppContext->EditorAssetManager->SaveAsset(mEditorViewport->GetAssetDescriptor());
            }
        }
    }
    if (!mIsOpen) return false;
    return open;
}

void Plu::IEditorPanel::EndPanel()
{
    ImGui::End();
    if (!mFillsOwnWindow && !ImGui::GetWindowDockNode()) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }    
}

void Plu::IEditorPanel::PanelChangedAsset() const
{
    mEditorViewport->ViewportChangedAsset();
}

Plu::IEditorPanel::IEditorPanel()
= default;

void Plu::IEditorPanel::Initialize(const TUsePointer<IEditorViewport> &viewport, bool canClose)
{
    mEditorViewport = viewport;
    mCanClose = canClose;
}

Plu::String Plu::IEditorPanel::GetPanelTitle()
{
    return GetPanelName() + "##" + GetParentViewport()->GetWindowName();
}
