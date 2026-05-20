//
// Created by Plutex on 2026-02-15.
//

#include "SceneWorldSettings.h"

#include "EditorAppContext.h"
#include "glm/gtc/type_ptr.hpp"
#include "Managers/Scene/EditorCamera.h"
#include "Managers/Scene/EditorScenesManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Renderer/Renderer.h"

extern Plu::ApplicationInfo* gApplicationInfo;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::String Plu::SceneWorldSettings::GetPanelName()
{
	return "World Settings";
}

void Plu::SceneWorldSettings::OnClosed()
{
}

void Plu::SceneWorldSettings::OnOpened()
{
}

void Plu::SceneWorldSettings::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		TUsePointer<SceneInfo> scene = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
		if (scene && gEditorAppContext->EditorScenesManager->IsAnySceneOpen()) {
			TypeSerializer<TClassPointer<GameMode>>::EditorControl(
				&gEditorAppContext->EditorScenesManager->GetCurrentWorld()->GameModeClass,
				"GameModeClass");
		}

		ImGui::Separator();
		ImGui::Text("World Stats");
		ImGui::Text("Physics Bodies: %d", gEditorAppContext->EditorScenesManager->GetCurrentWorld()->GetPhysicsWorld()->GetSystem().GetNumBodies());
		ImGui::Text("Renderables: %lu", gApplicationInfo->AppRenderer->NumOfRenderables());
		ImGui::Text("Camera Location: %f %f %f", gEditorAppContext->EditorScenesManager->SceneCamera->GetCameraLocation().x,
			gEditorAppContext->EditorScenesManager->SceneCamera->GetCameraLocation().y,
			gEditorAppContext->EditorScenesManager->SceneCamera->GetCameraLocation().z);
		ImGui::Text("Camera Rotation: %f %f %f", gEditorAppContext->EditorScenesManager->SceneCamera->GetCameraRotation().x,
			gEditorAppContext->EditorScenesManager->SceneCamera->GetCameraRotation().y,
			gEditorAppContext->EditorScenesManager->SceneCamera->GetCameraRotation().z);
		ImGui::Text("Camera Human Rotation: %f %f %f", gEditorAppContext->EditorScenesManager->SceneCamera->GetNiceRotation().x,
			gEditorAppContext->EditorScenesManager->SceneCamera->GetNiceRotation().y,
			gEditorAppContext->EditorScenesManager->SceneCamera->GetNiceRotation().z);
		ImGui::Separator();
		TypeSerializer<PhysicsDebugRender>::EditorControl(
			&gApplicationInfo->AppRenderer->PhysicsDebugRenderMode,
			"Physics Visualize Mode");
		ImGui::ColorEdit3("Wireframe Color", &static_cast<glm::vec3*>(&gApplicationInfo->AppRenderer->PhysicsDebugRenderColorWireframe)->x);
		ImGui::ColorEdit3("Points Color", &static_cast<glm::vec3*>(&gApplicationInfo->AppRenderer->PhysicsDebugRenderColorPoints)->x);
	}
	EndPanel();
}
