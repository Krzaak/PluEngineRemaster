#include "IEditorPanel.h"

#include <imgui_internal.h>

#include <utility>

#include "EditorAppContext.h"
#include "EditorViewportManager.h"
#include "PluEngine/Assets/EngineAssetManager.h"

extern Plu::EditorAppContext* gEditorAppContext;

bool Plu::IEditorPanel::BeginPanel(ImGuiWindowFlags flags)
{
    if (!ImGui::GetWindowDockNode()) {
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 1));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    }
    ImGui::SetNextWindowClass(GetParentViewport()->GetViewportWindowClass());
    bool open = ImGui::Begin(GetPanelTitle().CStr(), mCanClose ? &mIsOpen : nullptr, flags);
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
    if (!ImGui::GetWindowDockNode()) {
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
