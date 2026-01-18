//
// Created by Plutex on 1/14/26.
//

#include "SceneObjectDetailsPanel.h"

#include "EditorAppContext.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Scene/EditorScenesManager.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "UI/IconsFontAwesome7.h"

extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;

Plu::String Plu::SceneInspectorPanel::GetPanelName()
{
	return "Inspector";
}

void Plu::SceneInspectorPanel::OnClosed()
{
}

void Plu::SceneInspectorPanel::OnOpened()
{
}

void Plu::SceneInspectorPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		EditorAssetObject<SceneInfo>* scene = dynamic_cast<EditorAssetObject<SceneInfo>*>(GetParentViewport()->GetAssetObject().GetRaw());
		if (scene && gEditorAppContext->EditorScenesManager->IsAnySceneOpen() && gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObject))
		{
			TUsePointer<GameObject> obj = gEngineObjectManager->GetObjectAsUser<GameObject>(gEditorAppContext->EditorState.SelectedGameObject);
			ImGui::Text("%s", obj->GetClass()->TypeName.CStr());
		}
	}
	EndPanel();
}
