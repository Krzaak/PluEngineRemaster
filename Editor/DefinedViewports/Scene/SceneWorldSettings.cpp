//
// Created by Plutex on 2026-02-15.
//

#include "SceneWorldSettings.h"

#include "EditorAppContext.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Scene/EditorScenesManager.h"
#include "PluEngine/Managers/ScenesManager.h"

extern Plu::EditorAppContext* gEditorAppContext;

Plu::String Plu::SceneWorldSettings::GetPanelName()
{
	return "World Settings";
}

void Plu::SceneWorldSettings::OnClosed()
{
}

void Plu::SceneWorldSettings::OnOpened()
{
}

void Plu::SceneWorldSettings::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		EditorAssetObject<SceneInfo>* scene = dynamic_cast<EditorAssetObject<SceneInfo>*>(GetParentViewport()->GetAssetObject().GetRaw());
		if (scene && gEditorAppContext->EditorScenesManager->IsAnySceneOpen()) {
			TypeSerializer<TClassPointer<GameMode>>::EditorControl(&gEditorAppContext->EditorScenesManager->GetCurrentWorld()->GameModeClass, "GameModeClass");
		}
	}
	EndPanel();
}
