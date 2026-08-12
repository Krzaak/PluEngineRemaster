//
// Created by Plutex on 7/7/26.
//

#include "SkeletalMeshViewport.h"
#include "PluEngine/AssetCore/EngineAssetManager.h"

#include "EditorAppContext.h"
#include "EngineAssets.h"
#include "SkeletalMeshDetailsPanel.h"
#include "SkeletalMeshViewportPanel.h"
#include "DefinedViewports/Skeleton/SkeletonHierarchyPanel.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "Managers/Assets/EditorAssetManager.h"

void Plu::EditorSkeletalMeshObject::OnSetupComponents()
{
	MeshComponent = AddComponent(SkeletalMeshComponent::GetStaticClass(), "EditorSkeletalMeshComponent");
}

void Plu::EditorAttachPointPreviewObject::OnSetupComponents()
{
	MeshComponent = AddComponent(StaticMeshComponent::GetStaticClass(), "AttachPointPreviewMeshComponent");
}

void Plu::SkeletalMeshViewport::OnOpened()
{
	AddPanel(SkeletalMeshDetailsPanel::GetStaticClass(), false);
	AddPanel(SkeletalMeshViewportPanel::GetStaticClass(), false);
	// Reuse the exact hierarchy tree from SkeletonViewport. Hidden until "Show Hierarchy" is on
	// (the details panel drives its Visible flag); docked on the left in OnPanelRegister.
	AddPanel(SkeletonHierarchyPanel::GetStaticClass(), false);

	// Musi być materiał na programie ze skeletal vertex shaderem (czyta SSBO BoneMatrices) —
	// zwykły BasicColorMaterial (DebugShader) renderuje skeletal mesh zamrożony w bind pose.
	Material = mEditorAppContext->EditorAssetManager->GetAssetData(EngineAssets::SkeletalMeshViewportMaterial);
	// Attach point previews są zwykłymi static meshami — muszą dostać materiał na statycznym
	// programie, inaczej skeletal vertex shader czytałby nieistniejące dane kości.
	AttachPointPreviewMaterial = mEditorAppContext->EditorAssetManager->GetAssetData(EngineAssets::StaticMeshViewportMaterial);
}

void Plu::SkeletalMeshViewport::OnClosed()
{
}

void Plu::SkeletalMeshViewport::OnPanelRegister()
{
	SkeletalMeshDetailsPanel* detailsPanel = GetPanelSlow<SkeletalMeshDetailsPanel>();
	SkeletalMeshViewportPanel* viewportPanel = GetPanelSlow<SkeletalMeshViewportPanel>();
	SkeletonHierarchyPanel* hierarchyPanel = GetPanelSlow<SkeletonHierarchyPanel>();
	if (detailsPanel && viewportPanel && hierarchyPanel) {
		// Match the panel's initial visibility to the toggle so it doesn't flash for one frame
		// before the details panel syncs it (panel update order within the viewport isn't fixed).
		hierarchyPanel->Visible = ShowHierarchy;

		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		// Viewport (left/main) | right column split vertically: Properties on top (30%, it's
		// short), Hierarchy filling the rest (70%). Hierarchy starts hidden, so ImGui merges its
		// empty slot and Properties reclaims the whole right column until "Show Hierarchy" is on.
		ImGuiID center = dockspaceID;
		ImGuiID rightNode = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.28f, nullptr, &center);
		ImGuiID rightBottom = ImGui::DockBuilderSplitNode(rightNode, ImGuiDir_Down, 0.7f, nullptr, &rightNode);

		ImGui::DockBuilderDockWindow(viewportPanel->GetPanelTitle().CStr(), center);
		ImGui::DockBuilderDockWindow(detailsPanel->GetPanelTitle().CStr(), rightNode);
		ImGui::DockBuilderDockWindow(hierarchyPanel->GetPanelTitle().CStr(), rightBottom);
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
