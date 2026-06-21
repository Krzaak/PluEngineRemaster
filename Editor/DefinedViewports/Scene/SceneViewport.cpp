//
// Created by Plutex on 1/13/26.
//

#include "SceneViewport.h"

#include "EditorAppContext.h"
#include "SceneObjectDetailsPanel.h"
#include "SceneStructurePanel.h"
#include "SceneViewportPanel.h"
#include "SceneWorldSettings.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"

extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::TUsePointer<Plu::EngineObjectManager> gEngineObjectManager;

void Plu::SceneViewport::OnInit()
{
	TUsePointer<SceneInfo> scene = gEditorAppContext->EditorAssetManager->GetAssetData(GetAssetDescriptor());
	mEditorAppContext->EditorScenesManager->ConnectToWorld(scene->URL, false);
}

void Plu::SceneViewport::OnClosed()
{
	gEditorAppContext->EditorScenesManager->DisconnectFromWorld();
}

void Plu::SceneViewport::OnOpened()
{
	AddPanel(SceneStructurePanel::GetStaticClass(), false);
	AddPanel(SceneViewportPanel::GetStaticClass(), false);
	AddPanel(SceneWorldSettings::GetStaticClass(), false);
	AddPanel(SceneInspectorPanel::GetStaticClass(), false);
}

void Plu::SceneViewport::OnPanelRegister()
{
	SceneStructurePanel* sceneDetailsPanel = GetPanelSlow<SceneStructurePanel>();
	SceneViewportPanel* sceneViewport = GetPanelSlow<SceneViewportPanel>();
	SceneInspectorPanel* sceneInspector = GetPanelSlow<SceneInspectorPanel>();
	SceneWorldSettings* sceneWorldSettings = GetPanelSlow<SceneWorldSettings>();
	if (sceneDetailsPanel && sceneViewport && sceneInspector && sceneWorldSettings)
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
		ImGui::DockBuilderDockWindow(sceneWorldSettings->GetPanelTitle().CStr(), rightDown);
		ImGui::DockBuilderDockWindow(sceneInspector->GetPanelTitle().CStr(), rightDown);
		ImGui::DockBuilderDockWindow(sceneViewport->GetPanelTitle().CStr(), left);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::SceneViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		if (ImGui::IsKeyDown(ImGuiKey_Delete)) {
			TUsePointer<SceneInfo> scene = gEditorAppContext->EditorAssetManager->GetAssetData(GetAssetDescriptor());
			if (scene && gEditorAppContext->EditorScenesManager->IsAnySceneOpen() && gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObject)) {
				TUsePointer<GameObject> gameObj = gEngineObjectManager->GetObjectAsUser<GameObject>(gEditorAppContext->EditorState.SelectedGameObject);
				gEditorAppContext->EditorScenesManager->GetCurrentWorld()->DeleteGameObject(*gameObj->GetEngineObjectHandle());
				gEditorAppContext->EditorState.SelectedGameObject = EngineObjectHandle();
				gEditorAppContext->EditorState.SelectedGameObjectComponent = EngineObjectHandle();
			}
		}
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
