//
// Created by Plutex on 2026-02-15.
//

#include "SceneWorldSettings.h"

#include "EditorAppContext.h"
#include "glm/gtc/type_ptr.hpp"
#include "Managers/Scene/EditorCamera.h"
#include "PluEngine/Application.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Physics/PhysicsWorld.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"

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
		ImGui::Separator();
		TypeSerializer<PhysicsDebugRender>::EditorControl(
			&gApplicationInfo->AppRenderer->PhysicsDebugRenderMode,
			"Physics Visualize Mode");
		ImGui::ColorEdit3("Wireframe Color", &static_cast<glm::vec3*>(&gApplicationInfo->AppRenderer->PhysicsDebugRenderColorWireframe)->x);
		ImGui::ColorEdit3("Points Color", &static_cast<glm::vec3*>(&gApplicationInfo->AppRenderer->PhysicsDebugRenderColorPoints)->x);
	}
	EndPanel();
}
