//
// Created by Plutex on 8/5/26.
//

#ifndef PLUENGINE_EDITORWINDOWMOVEMENU_H
#define PLUENGINE_EDITORWINDOWMOVEMENU_H

#include <functional>
#include <imgui.h>

#include "EditorAppContext.h"
#include "EditorWindows/EditorWindowsManager.h"

extern Plu::EditorAppContext* gEditorAppContext;

namespace Plu
{
    // "Move To Window" submenu, shared by panels, viewports and viewport panels. Call it right
    // after ImGui::Begin() inside a BeginPopupContextItem(): ImGui sets the "last item" of a window
    // to its dock tab (or title bar), so that popup is exactly "right-click on the tab".
    //
    // kindFilter says what kind of window this thing can live in — a viewport panel goes into its
    // own SinglePanel window, everything else into a Dockspace one. onMove receives the target
    // window id, already created when the user picked "New Window".
    inline void DrawMoveToWindowMenu(UInt32 currentWindowID, EEditorWindowKind kindFilter,
                                     const String& newWindowTitle,
                                     const std::function<void(UInt32)>& onMove)
    {
        if (!gEditorAppContext || !gEditorAppContext->EditorWindowsManager) return;
        if (!ImGui::BeginMenu("Move To Window")) return;

        if (ImGui::MenuItem("Move to New Window")) {
            const UInt32 newWindowID = gEditorAppContext->EditorWindowsManager->CreateEditorWindow(kindFilter, newWindowTitle);
            if (newWindowID != 0) onMove(newWindowID);
        }

        bool separatorDrawn = false;
        for (const EditorWindowInfo* window : gEditorAppContext->EditorWindowsManager->GetWindows()) {
            if (window->WindowID == currentWindowID) continue;
            // A SinglePanel window holds exactly one panel, so it is never a destination — the only
            // way into one is "Move to New Window".
            if (window->Kind == EEditorWindowKind::SinglePanel) continue;
            if (!separatorDrawn) {
                ImGui::Separator();
                separatorDrawn = true;
            }
            if (ImGui::MenuItem(window->Title.CStr())) {
                onMove(window->WindowID);
            }
        }

        ImGui::EndMenu();
    }
}

#endif //PLUENGINE_EDITORWINDOWMOVEMENU_H
