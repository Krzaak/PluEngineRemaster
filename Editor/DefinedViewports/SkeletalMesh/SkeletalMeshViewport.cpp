//
// Created by Plutex on 7/7/26.
//

#include "SkeletalMeshViewport.h"

#include "EditorAppContext.h"
#include "EngineAssets.h"
#include "SkeletalMeshDetailsPanel.h"
#include "SkeletalMeshViewportPanel.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "Managers/Assets/EditorAssetManager.h"

void Plu::EditorSkeletalMeshObject::OnSetupComponents()
{
	MeshComponent = AddComponent(SkeletalMeshComponent::GetStaticClass(), "EditorSkeletalMeshComponent");
}

void Plu::SkeletalMeshViewport::OnOpened()
{
	AddPanel(SkeletalMeshDetailsPanel::GetStaticClass(), false);
	AddPanel(SkeletalMeshViewportPanel::GetStaticClass(), false);

	// Musi być materiał na programie ze skeletal vertex shaderem (czyta SSBO BoneMatrices) —
	// zwykły BasicColorMaterial (DebugShader) renderuje skeletal mesh zamrożony w bind pose.
	Material = mEditorAppContext->EditorAssetManager->GetAssetData(EngineAssets::BasicColorSkeletalMaterial);
}

void Plu::SkeletalMeshViewport::OnClosed()
{
}

void Plu::SkeletalMeshViewport::OnPanelRegister()
{
	SkeletalMeshDetailsPanel* detailsPanel = GetPanelSlow<SkeletalMeshDetailsPanel>();
	SkeletalMeshViewportPanel* viewportPanel = GetPanelSlow<SkeletalMeshViewportPanel>();
	if (detailsPanel && viewportPanel) {
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		ImGuiID left, right;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left, 0.7f, &left, &right);

		ImGui::DockBuilderDockWindow(viewportPanel->GetPanelTitle().CStr(), left);
		ImGui::DockBuilderDockWindow(detailsPanel->GetPanelTitle().CStr(), right);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::SkeletalMeshViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
