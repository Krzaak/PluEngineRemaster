//
// Created by Plutex on 5/30/26.
//

#include "ProjectSettingsPanel.h"

#include "EditorAppContext.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluGame.h"
#include "PluEngine/Reflection/TypeTraits.h"
#include "UI/IconsFontAwesome7.h"

Plu::String Plu::ProjectSettingsPanel::GetPanelName()
{
    return ICON_FA_GEARS " Project Settings";
}

void Plu::ProjectSettingsPanel::OnHide()
{
}

void Plu::ProjectSettingsPanel::OnShow()
{
}

void Plu::ProjectSettingsPanel::OnUpdate(float deltaTime)
{
    if (BeginPanel()) {
        TypeSerializer<TypeInfo*>::EditorControl(GameStartupSettings::GetStaticClass(), mEditorAppContext->EditorProjectManager->GetGameStartupSettings().GetRaw());
    }
    EndPanel();
}
