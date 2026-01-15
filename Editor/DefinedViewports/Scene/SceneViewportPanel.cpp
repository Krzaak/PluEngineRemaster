//
// Created by Plutex on 1/14/26.
//

#include "SceneViewportPanel.h"

#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Scene/EditorScene.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"

Plu::String Plu::SceneViewportPanel::GetPanelName()
{
	return "Viewport";
}

void Plu::SceneViewportPanel::OnClosed()
{
}

void Plu::SceneViewportPanel::OnOpened()
{
}

void Plu::SceneViewportPanel::OnUpdate(float deltaTime)
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
