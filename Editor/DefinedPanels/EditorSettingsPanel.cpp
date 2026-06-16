//
// Created by Plutex on 5/3/26.
//

#include "EditorSettingsPanel.h"

#include "EditorSettings/EditorSettings.h"
#include "EditorSettings/EditorSettingsManager.h"
#include "PluEngine/PluUtils.h"

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
        for (auto prop : EditorSettings::GetStaticClass()->Properties) {
            prop->EditorControlPtr(prop->GetPtr(EditorSettingsManager::GetInstance()->GetSettings()), MakeStringForDisplay(prop->PropertyName));
        }
    }
    EndPanel();
}
