//
// Created by Plutex on 7/6/26.
//

#include "SkeletonViewport.h"

#include "SkeletonHierarchyPanel.h"
#include "SkeletonViewportPanel.h"
#include "Managers/Scene/EditorCamera.h"
#include "PluEngine/Objects/EngineObjectManager.h"

void Plu::SkeletonViewport::OnInit()
{
	// Skeleton is a binary asset, so Ctrl+S / "Save All" are off unless asked for explicitly —
	// attach point edits are asset data and have to be saveable from here.
	SetCanBeSaved(true);

	// Dedicated navigation camera, independent of the shared editor scene camera so moving
	// around a skeleton never disturbs the scene view. Destroyed in OnClosed().
	EngineObjectHandle hdl = mEngineObjectManager->CreateObject<EditorSceneCamera>();
	Camera = mEngineObjectManager->GetObjectAsOwner<EditorSceneCamera>(hdl);
}

void Plu::SkeletonViewport::OnClosed()
{
	// OnClosed is called exactly once by EditorViewportManager before the viewport object is
	// destroyed (both on close and on editor shutdown), so this is the clean teardown point.
	if (Camera) {
		mEngineObjectManager->DestroyObject(*Camera->GetEngineObjectHandle());
		Camera = nullptr;
	}
}

void Plu::SkeletonViewport::OnOpened()
{
	AddPanel(SkeletonHierarchyPanel::GetStaticClass(), false);
	AddPanel(SkeletonViewportPanel::GetStaticClass(), false);
}

void Plu::SkeletonViewport::OnPanelRegister()
{
	SkeletonHierarchyPanel* hierarchyPanel = GetPanelSlow<SkeletonHierarchyPanel>();
	SkeletonViewportPanel* viewportPanel = GetPanelSlow<SkeletonViewportPanel>();
	if (hierarchyPanel && viewportPanel) {
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		ImGuiID left, right;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left, 0.3f, &left, &right);

		ImGui::DockBuilderDockWindow(hierarchyPanel->GetPanelTitle().CStr(), left);
		ImGui::DockBuilderDockWindow(viewportPanel->GetPanelTitle().CStr(), right);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::SkeletonViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
