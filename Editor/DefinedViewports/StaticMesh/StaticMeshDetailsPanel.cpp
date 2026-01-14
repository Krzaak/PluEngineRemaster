//
// Created by Plutex on 1/14/26.
//

#include "StaticMeshDetailsPanel.h"

#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"

Plu::String Plu::StaticMeshDetailsPanel::GetPanelName()
{
	return "Properties";
}

void Plu::StaticMeshDetailsPanel::OnClosed()
{
}

void Plu::StaticMeshDetailsPanel::OnOpened()
{
}

void Plu::StaticMeshDetailsPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		EditorAssetObject<StaticMesh>* staticMesh = dynamic_cast<EditorAssetObject<StaticMesh>*>(GetParentViewport()->GetAssetObject().GetRaw());
		if (staticMesh)
		{
			ImGui::Text("Vertices: %lu", staticMesh->AssetInfo.StaticMeshData.Vertices.Size());
			ImGui::Text("Indices: %lu", staticMesh->AssetInfo.StaticMeshData.Indices.Size());
		}
	}
	EndPanel();
}
