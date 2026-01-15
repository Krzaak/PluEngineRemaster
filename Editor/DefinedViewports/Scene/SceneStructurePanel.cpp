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
		EditorAssetObject<EditorScene>* scene = dynamic_cast<EditorAssetObject<EditorScene>*>(GetParentViewport()->GetAssetObject().GetRaw());
		if (scene)
		{
			
		}
	}
	EndPanel();
}
