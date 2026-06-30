//
// Created by Plutex on 5/3/26.
//

#include "EditorSettingsPanel.h"

#include "EditorSettings/EditorSettings.h"
#include "EditorSettings/EditorSettingsManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Window/Window.h"

Plu::String Plu::EditorSettingsPanel::GetPanelName()
{
    return "Settings";
}

void Plu::EditorSettingsPanel::OnHide()
{
}

void Plu::EditorSettingsPanel::OnShow()
{
}

void Plu::EditorSettingsPanel::OnUpdate(float deltaTime)
{
    if (BeginPanel()) {
        bool vsyncEnabled = mApplicationInfo->AppWindow->IsVSyncEnabled();
        if (ImGui::Checkbox("VSync", &vsyncEnabled)) {
            mApplicationInfo->AppWindow->SetVSyncEnabled(vsyncEnabled);
        }
        ImGui::Separator();
        for (auto prop : EditorSettings::GetStaticClass()->Properties) {
            prop->EditorControlPtr(prop->GetPtr(EditorSettingsManager::GetInstance()->GetSettings()), MakeStringForDisplay(prop->PropertyName));
        }
    }
    EndPanel();
}
