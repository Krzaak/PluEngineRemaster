//
// Created by Plutex on 5/30/26.
//

#include "ProjectSettingsPanel.h"

#include "EditorAppContext.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/Gameplay/PluGame.h"
#include "PluEngine/Core/CollisionChannels.h"
#include "PluEngine/Core/Reflection/TypeTraits.h"
#include "UI/IconsFontAwesome7.h"
#include "String/String.h"
#include "Array/Array.h"
#include <cstdio>
#include <cfloat>

Plu::String Plu::ProjectSettingsPanel::GetPanelName()
{
    return ICON_FA_GEARS " Project Settings";
}

void Plu::ProjectSettingsPanel::OnHide()
{
}

void Plu::ProjectSettingsPanel::OnShow()
{
    SetCanClose(true);
}

void Plu::ProjectSettingsPanel::OnUpdate(float deltaTime)
{
    if (BeginPanel()) {
        if (!mEditorAppContext->EditorProjectManager->IsAnyProjectOpen())
        {
            ImGui::Text("Open Project before browsing settings!");
            EndPanel();
            return;
        }
        TypeSerializer<TypeInfo*>::EditorControl(GameStartupSettings::GetStaticClass(), mEditorAppContext->EditorProjectManager->GetGameStartupSettings().GetRaw());
    }
    EndPanel();
}

namespace
{
    // Tint per response (Ignore / Overlap / Block) — reused for headers + radio checkmarks.
    const ImVec4 kResponseColors[3] = {
        ImVec4(0.78f, 0.42f, 0.42f, 1.0f), // Ignore  — muted red
        ImVec4(0.90f, 0.78f, 0.34f, 1.0f), // Overlap — amber
        ImVec4(0.45f, 0.80f, 0.48f, 1.0f), // Block   — green
    };
    const char* kResponseNames[3] = { "Ignore", "Overlap", "Block" };
}
