//
// Created by Plutex on 1/14/26.
//

#include "StaticMeshDetailsPanel.h"

#include "StaticMeshViewport.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"
#include "PluEngine/BasicEngineClasses/GameObjects/MeshObject.h"
#include "PluEngine/Managers/ScenesManager.h"

extern Plu::ApplicationInfo* gApplicationInfo;
extern Plu::EditorAppContext* gEditorAppContext;

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
		TUsePointer<StaticMeshViewport> parentMeshViewport = DynamicCast<StaticMeshViewport>(GetParentViewport());
		TUsePointer<MaterialInfo> before = parentMeshViewport->Material;
		TypeSerializer<TUsePointer<MaterialInfo>>::EditorControl(&parentMeshViewport->Material, "Material");
		if (before != parentMeshViewport->Material) {
			TUsePointer<EditorMeshObject> meshObject = gApplicationInfo->AppScenesManager->GetCurrentWorld()->GetGameObjectOfClass(EditorMeshObject::GetStaticClass());
			meshObject->MeshComponent->SetMaterial(parentMeshViewport->Material);
		}
		TUsePointer<StaticMesh> staticMesh = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
		if (staticMesh)
		{
			ImGui::Text("Vertices: %lu", staticMesh->StaticMeshData.Vertices.Size());
			ImGui::Text("Indices: %lu", staticMesh->StaticMeshData.Indices.Size() / 3);
		}
	}
	EndPanel();
}
