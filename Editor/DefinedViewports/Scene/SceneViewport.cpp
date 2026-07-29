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
	mEditorAppContext->EditorScenesManager->GetBaseSceneWorld()->GetObjectEventDispatcher()->Unsubscribe("GameObjectsChanged", mGameObjectsChangedHandle);
	// OnOpened runs on every open, so without this a reopened viewport would stack handlers and each
	// reloaded class would be processed once per stacked subscription.
	TypeRegistry::GetInstance()->TypeRegistryEventDispatcher.Unsubscribe("NewPythonType", mNewPythonTypeHandle);
	mPendingPythonTypeReloads.Clear();
	gEditorAppContext->EditorScenesManager->DisconnectFromWorld();
}

void Plu::SceneViewport::OnOpened()
{
	AddPanel(SceneStructurePanel::GetStaticClass(), false);
	AddPanel(SceneViewportPanel::GetStaticClass(), false);
	AddPanel(SceneInspectorPanel::GetStaticClass(), false);
	AddPanel(SceneWorldSettings::GetStaticClass(), false);

	mGameObjectsChangedHandle = mEditorAppContext->EditorScenesManager->GetBaseSceneWorld()->GetObjectEventDispatcher()->Subscribe("GameObjectsChanged", [this](void*) {
		ViewportChangedAsset();
	});

	// Only queued here — see mPendingPythonTypeReloads for why the recreate pass cannot run from
	// inside the dispatch.
	mNewPythonTypeHandle = TypeRegistry::GetInstance()->TypeRegistryEventDispatcher.Subscribe("NewPythonType", [this](void* data) {
		String* typeName = static_cast<String *>(data);
		if (!typeName || typeName->IsEmpty()) return;
		if (mPendingPythonTypeReloads.Contains(*typeName)) return;
		mPendingPythonTypeReloads.PushBack(*typeName);
	});
}

void Plu::SceneViewport::FlushPendingPythonTypeReloads()
{
	if (mPendingPythonTypeReloads.IsEmpty()) return;
	DynamicArray<String> typeNames = std::move(mPendingPythonTypeReloads);
	mPendingPythonTypeReloads.Clear();

	TUsePointer<SceneManager> scenesManager = gEditorAppContext->EditorScenesManager;
	if (!scenesManager || !scenesManager->IsAnySceneOpen()) return;
	if (scenesManager->IsInPIE()) {
		PLU_WARN("Python scripts reloaded, but live objects were left alone — stop PIE and reload again to apply the new code to them.");
		return;
	}

	// The recreated object is a different EngineObject, so the stored handle stops resolving. UUID for
	// the object, name for the component — the two keys that survive a recreate by design.
	UInt64 selectedUuid = 0;
	String selectedComponentName;
	if (gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObject)) {
		if (TUsePointer<GameObject> selected = gEngineObjectManager->GetObjectAsUser<GameObject>(gEditorAppContext->EditorState.SelectedGameObject)) {
			selectedUuid = selected->GetObjectUUID();
			if (gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObjectComponent)) {
				if (TUsePointer<GameObjectComponent> selectedComponent = gEngineObjectManager->GetObjectAsUser<GameObjectComponent>(gEditorAppContext->EditorState.SelectedGameObjectComponent)) {
					selectedComponentName = selectedComponent->GetComponentName();
				}
			}
		}
	}

	scenesManager->ReloadPythonInstances(typeNames);

	if (selectedUuid == 0) return;
	TUsePointer<SceneWorld> world = scenesManager->GetBaseSceneWorld();
	TUsePointer<GameObject> restored = world ? world->GetGameObjectByUUID(PluUUID(selectedUuid)) : nullptr;
	if (!restored) {
		// The object was python and its class failed to come back (script error) — the details panel
		// must not keep editing a corpse.
		gEditorAppContext->EditorState.SelectedGameObject = EngineObjectHandle();
		gEditorAppContext->EditorState.SelectedGameObjectComponent = EngineObjectHandle();
		return;
	}
	gEditorAppContext->EditorState.SelectedGameObject = *restored->GetEngineObjectHandle();
	gEditorAppContext->EditorState.SelectedGameObjectComponent = EngineObjectHandle();
	if (selectedComponentName.IsEmpty()) return;
	for (const auto& worldComponent : *restored->GetObjectWorldComponents()) {
		if (!worldComponent || worldComponent->GetComponentName() != selectedComponentName) continue;
		gEditorAppContext->EditorState.SelectedGameObjectComponent = *worldComponent->GetEngineObjectHandle();
		return;
	}
	for (const auto& component : *restored->GetObjectComponents()) {
		if (!component || component->GetComponentName() != selectedComponentName) continue;
		gEditorAppContext->EditorState.SelectedGameObjectComponent = *component->GetEngineObjectHandle();
		return;
	}
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
		ImGui::DockBuilderSetNodeSize(dockspaceID, GetDockspaceSize());

		ImGuiID left, right;
		ImGuiID rightDown, rightUp;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Left, 0.8f, &left, &right);
		ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 0.5f, &rightUp, &rightDown);
		ImGui::DockBuilderDockWindow(sceneDetailsPanel->GetPanelTitle().CStr(), rightUp);
		ImGui::DockBuilderDockWindow(sceneInspector->GetPanelTitle().CStr(), rightDown);
		ImGui::DockBuilderDockWindow(sceneWorldSettings->GetPanelTitle().CStr(), rightDown);
		ImGui::DockBuilderDockWindow(sceneViewport->GetPanelTitle().CStr(), left);
		ImGui::DockBuilderFinish(dockspaceID);

		// Inspector and World Settings share rightDown as tabs. Focus the Inspector on its next Begin
		// so it's the active tab on open instead of hiding behind World Settings.
		sceneInspector->BringToFront();
	}
}

void Plu::SceneViewport::OnUpdate(float deltaTime)
{
	// Outside BeginWindow: the scripts are reloaded from a menu item, and the objects have to be
	// recreated whether or not this window happens to be visible this frame.
	FlushPendingPythonTypeReloads();
	if (BeginWindow()) {
		if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
			TUsePointer<SceneInfo> scene = gEditorAppContext->EditorAssetManager->GetAssetData(GetAssetDescriptor());
			if (scene && gEditorAppContext->EditorScenesManager->IsAnySceneOpen()) {
				if (gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObjectComponent) &&
					gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObject)) {
					TUsePointer<GameObject> selected = gEngineObjectManager->GetObjectAsUser<GameObject>(gEditorAppContext->EditorState.SelectedGameObject);
					selected->DeleteComponent(gEngineObjectManager->GetObjectAsUser<GameObjectComponent>(gEditorAppContext->EditorState.SelectedGameObjectComponent).GetRaw());
					gEditorAppContext->EditorState.SelectedGameObjectComponent = EngineObjectHandle();
				} else if (gEngineObjectManager->IsValid(gEditorAppContext->EditorState.SelectedGameObject)) {
					// Multi-selection żyje w Structure panelu (EditorState trzyma tylko primary),
					// więc kasowanie oddajemy jemu. GetPanelSlow leci tylko z wciśniętym Delete.
					if (SceneStructurePanel* structurePanel = GetPanelSlow<SceneStructurePanel>()) {
						structurePanel->DeleteSelectedObjects();
					} else {
						TUsePointer<GameObject> gameObj = gEngineObjectManager->GetObjectAsUser<GameObject>(gEditorAppContext->EditorState.SelectedGameObject);
						gEditorAppContext->EditorScenesManager->GetCurrentWorld()->DeleteGameObject(*gameObj->GetEngineObjectHandle());
						gEditorAppContext->EditorState.SelectedGameObject = EngineObjectHandle();
						gEditorAppContext->EditorState.SelectedGameObjectComponent = EngineObjectHandle();
					}
				}
			}
		}
		UpdatePanels(deltaTime);
	}
	EndWindow();
}
