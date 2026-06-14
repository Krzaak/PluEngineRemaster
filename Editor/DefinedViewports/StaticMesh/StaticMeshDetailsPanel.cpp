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
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Assets/AssetLoaders/StaticMesh/StaticMeshAssimpLoader.h"
#include "PluEngine/Assets/AssetDescriptor.h"

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
			ImGui::Text("Triangles: %lu", staticMesh->StaticMeshData.Indices.Size() / 3);

			ImGui::Separator();

			if (ImGui::CollapsingHeader("Collision", ImGuiTreeNodeFlags_DefaultOpen))
			{
				bool showCollision = parentMeshViewport->ShowCollision;
				if (ImGui::Checkbox("Show Wireframe", &showCollision))
					parentMeshViewport->ShowCollision = showCollision;

				ImGui::Spacing();
				ImGui::Text("Collision Shapes (%zu):", staticMesh->CollisionShapes.Size());
				ImGui::Separator();

				int removeIdx = -1;
				for (int i = 0; i < (int)staticMesh->CollisionShapes.Size(); i++)
				{
					const auto& def = staticMesh->CollisionShapes[i];
					ImGui::PushID(i);

					const char* typeName = def.Type == StaticMeshCollisionType::PerVertex ? "PerVertex" : "Approximate";
					const char* modeName = "";
					if (def.Type == StaticMeshCollisionType::Approximate)
					{
						switch (def.ApproxMode)
						{
						case ApproximateCollisionMode::BoundingBox: modeName = " (BoundingBox)"; break;
						case ApproximateCollisionMode::ConvexHull:  modeName = " (ConvexHull)";  break;
						case ApproximateCollisionMode::Sphere:      modeName = " (Sphere)";      break;
						}
					}
					ImGui::Text("[%d] %s%s", i, typeName, modeName);
					ImGui::SameLine();
					if (ImGui::SmallButton("Remove"))
						removeIdx = i;

					ImGui::PopID();
				}

				if (removeIdx >= 0)
				{
					staticMesh->CollisionShapes.RemoveAt(removeIdx);
					MeshImporter::SaveStaticMesh(GetParentViewport()->GetAssetDescriptor()->AssetPath.ToString().ToWide(), staticMesh.GetRaw());
					parentMeshViewport->CollisionDirty = true;
				}

				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Text("Generate New:");

				static int sTypeIdx = 0;
				static int sModeIdx = 1; // ConvexHull default
				const char* typeItems[] = { "Approximate", "PerVertex" };
				const char* modeItems[] = { "BoundingBox", "ConvexHull", "Sphere" };

				ImGui::Combo("Type", &sTypeIdx, typeItems, 2);
				if (sTypeIdx == 0)
					ImGui::Combo("Mode", &sModeIdx, modeItems, 3);

				if (sTypeIdx == 1)
					ImGui::TextDisabled("Note: PerVertex only works on Static bodies.");

				if (ImGui::Button("Generate & Add"))
				{
					StaticMeshCollisionDef def;
					def.Type = sTypeIdx == 0 ? StaticMeshCollisionType::Approximate : StaticMeshCollisionType::PerVertex;
					def.ApproxMode = static_cast<ApproximateCollisionMode>(sModeIdx);
					staticMesh->CollisionShapes.PushBack(def);
					MeshImporter::SaveStaticMesh(GetParentViewport()->GetAssetDescriptor()->AssetPath.ToString().ToWide(), staticMesh.GetRaw());
					parentMeshViewport->CollisionDirty = true;
				}
			}
		}
	}
	EndPanel();
}
