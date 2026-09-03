//
// Created by Plutex on 2026-02-15.
//

#include "SceneWorldSettings.h"

#include "EditorAppContext.h"
#include "glm/gtc/type_ptr.hpp"
#include "Managers/Scene/EditorCamera.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetCore/EngineAssetManager.h"
#include "PluEngine/Gameplay/Scenes/ScenesManager.h"
#include "PluEngine/Gameplay/PhysicsWorld.h"
#include "PluEngine/Render/Renderer.h"
#include "PluEngine/Gameplay/Scenes/SceneManager.h"
#include "PluEngine/Gameplay/Scenes/SceneWorld.h"

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
			if (TypeSerializer<TClassPointer<GameMode>>::EditorControl(
				&gEditorAppContext->EditorScenesManager->GetCurrentWorld()->GameModeClass,
				"GameModeClass")) {
				PanelChangedAsset();
			}
		}

		ImGui::Separator();
		ImGui::Text("World Stats");
		PhysicsWorld* physicsWorld = gEditorAppContext->EditorScenesManager->GetCurrentWorld()->GetPhysicsWorld();
		ImGui::Text("Physics Bodies: %d", physicsWorld->GetSystem().GetNumBodies());
		ImGui::Separator();
		// Ustawienia wizualizacji debugowej fizyki żyją teraz na PhysicsWorld (czytane na main
		// przy budowie snapshotu), bo Renderer jest obiektem wyłącznie wątku renderu.
		if (TypeSerializer<PhysicsDebugRender>::EditorControl(
			&physicsWorld->PhysicsDebugRenderMode,
			"Physics Visualize Mode")) {
			PanelChangedAsset();
		}
		if (ImGui::ColorEdit3("Wireframe Color", &physicsWorld->PhysicsDebugRenderColorWireframe.x)) {
			PanelChangedAsset();
		}
		if (ImGui::ColorEdit3("Points Color", &physicsWorld->PhysicsDebugRenderColorPoints.x)) {
			PanelChangedAsset();
		}

		ImGui::Separator();
		// Editor grid — view-only state (like the viewport camera), deliberately not dirtying
		// the asset. Read on main by RenderSnapshotBuilder; hidden automatically during PIE.
		TUsePointer<SceneWorld> world = gEditorAppContext->EditorScenesManager->GetCurrentWorld();
		ImGui::Checkbox("Show Editor Grid", &world->ShowEditorGrid);
		ImGui::Checkbox("Show Shadow Cascades", &world->ShowShadowCascades);
		ImGui::SetItemTooltip("Tints each pixel by the shadow cascade it samples. The gradients between bands are the cascade blend zones.");
	}
	EndPanel();
}
