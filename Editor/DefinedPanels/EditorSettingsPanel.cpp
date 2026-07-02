//
// Created by Plutex on 5/3/26.
//

#include "EditorSettingsPanel.h"

#include "EditorSettings/EditorSettings.h"
#include "EditorSettings/EditorSettingsManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Managers/RenderingManager.h"
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
    SetCanClose(true);
}

void Plu::EditorSettingsPanel::OnUpdate(float deltaTime)
{
    if (BeginPanel()) {
        bool vsyncEnabled = mApplicationInfo->AppWindow->IsVSyncEnabled();
        if (ImGui::Checkbox("VSync", &vsyncEnabled)) {
            mApplicationInfo->AppWindow->SetVSyncEnabled(vsyncEnabled);
        }
        bool limitTextureLoadPerFrame = mApplicationInfo->AppRenderingManager->IsLimitTextureLoadPerFrame();
        if (ImGui::Checkbox("Limit Amount of Textures loaded per frame", &limitTextureLoadPerFrame)) {
            mApplicationInfo->AppRenderingManager->SetLimitTextureLoadPerFrame(limitTextureLoadPerFrame);
        }
        ImGui::SetItemTooltip("Purely for fun, I just think it looks cool when textures seem to load frame by frame (In runtime this will not work :( Performance they said)");
        ImGui::Separator();
        for (auto prop : EditorSettings::GetStaticClass()->Properties) {
            prop->EditorControlPtr(prop->GetPtr(EditorSettingsManager::GetInstance()->GetSettings()), MakeStringForDisplay(prop->PropertyName));
        }
    }
    EndPanel();
}
