//
// Created by Plutex on 1/14/26.
//

#include "SceneStructurePanel.h"

#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Scene/EditorScene.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"

Plu::String Plu::SceneStructurePanel::GetPanelName()
{
	return "Structure";
}

void Plu::SceneStructurePanel::OnClosed()
{
}

void Plu::SceneStructurePanel::OnOpened()
{
}

void Plu::SceneStructurePanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		EditorAssetObject<SceneInfo>* scene = dynamic_cast<EditorAssetObject<SceneInfo>*>(GetParentViewport()->GetAssetObject().GetRaw());
		if (scene)
		{
			
		}
	}
	EndPanel();
}
