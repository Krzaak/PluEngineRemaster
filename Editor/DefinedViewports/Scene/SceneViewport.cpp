//
// Created by Plutex on 1/13/26.
//

#include "SceneViewport.h"

#include "EditorAppContext.h"
#include "SceneObjectDetailsPanel.h"
#include "SceneStructurePanel.h"
#include "SceneViewportPanel.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Scene/EditorScenesManager.h"

void Plu::SceneViewport::OnInit()
{
	EditorAssetObject<SceneInfo>* scene = dynamic_cast<EditorAssetObject<SceneInfo>*>(GetAssetObject().GetRaw());
	mEditorAppContext->EditorScenesManager->PrepareWorldForEditor(scene->AssetInfo.URL);
}

void Plu::SceneViewport::OnClosed()
{
}

void Plu::SceneViewport::OnOpened()
{
	AddPanel(SceneStructurePanel::GetStaticClass(), false);
	AddPanel(SceneViewportPanel::GetStaticClass(), false);
	AddPanel(SceneInspectorPanel::GetStaticClass(), false);
}

void Plu::SceneViewport::OnPanelRegister()
{
	SceneStructurePanel* sceneDetailsPanel = GetPanelSlow<SceneStructurePanel>();
	SceneViewportPanel* sceneViewport = GetPanelSlow<SceneViewportPanel>();
	SceneInspectorPanel* sceneInspector = GetPanelSlow<SceneInspectorPanel>();
	if (sceneDetailsPanel && sceneViewport && sceneInspector)
	{
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		ImGuiID left, right;
		ImGuiID rightDown, rightUp;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left, 0.7f, &left, &right);
		ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 0.5f, &rightUp, &rightDown);
		ImGui::DockBuilderDockWindow(sceneDetailsPanel->GetPanelTitle().CStr(), rightUp);
		ImGui::DockBuilderDockWindow(sceneInspector->GetPanelTitle().CStr(), rightDown);
		ImGui::DockBuilderDockWindow(sceneViewport->GetPanelTitle().CStr(), left);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::SceneViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
