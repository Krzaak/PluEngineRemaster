//
// Created by Plutex on 2026-03-05.
//

#include "StaticMeshViewportPanel.h"

#include <glad/glad.h>
#include "EditorAppContext.h"
#include "StaticMeshViewport.h"
#include "Managers/Scene/EditorCamera.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Renderer/GLFrameBuffer.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"
#include "PluEngine/BasicEngineClasses/GameObjects/MeshObject.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Physics/PhysicsWorld.h"

#include "PluEngine/Managers/RenderingManager.h"

extern Plu::ApplicationInfo* gApplicationInfo;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::String Plu::StaticMeshViewportPanel::GetPanelName()
{
	return "Viewport";
}

void Plu::StaticMeshViewportPanel::OnClosed()
{
}

void Plu::StaticMeshViewportPanel::OnOpened()
{
	PLU_INFO("Mesh Viewport Opened!");
	gEditorAppContext->EditorScenesManager->CreateOverlayScene();
	TUsePointer<SceneWorld> overlay = gEditorAppContext->EditorScenesManager->GetCurrentWorld();
	TUsePointer<EditorMeshObject> meshObject = overlay->SpawnGameObject(EditorMeshObject::GetStaticClass());
	TUsePointer<StaticMesh> staticMesh = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
	meshObject->MeshComponent->SetStaticMesh(staticMesh);
	TUsePointer<StaticMeshViewport> parentMeshViewport = DynamicCast<StaticMeshViewport>(GetParentViewport());
	meshObject->MeshComponent->SetMaterial(parentMeshViewport->Material);
}

void Plu::StaticMeshViewportPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		TUsePointer<StaticMeshViewport> parentMeshViewport = DynamicCast<StaticMeshViewport>(GetParentViewport());

		// Wizualizacja kolizji jedzie centralną ścieżką debug-geometrii: RenderSnapshotBuilder
		// na MAIN woła DrawEditModeShapes() na PhysicsWorld bieżącego świata (= overlay tego
		// viewportu, bo GetCurrentWorld() zwraca overlay), pakuje linie do snapshotu, a wątek
		// renderu je rysuje. Tu tylko sterujemy trybem z checkboxa i invalidujemy cache kształtów.
		TUsePointer<SceneWorld> overlay = gEditorAppContext->EditorScenesManager->GetCurrentWorld();
		PhysicsWorld* overlayPhysics = overlay ? overlay->GetPhysicsWorld() : nullptr;
		if (overlayPhysics)
		{
			overlayPhysics->PhysicsDebugRenderMode =
				parentMeshViewport->ShowCollision ? PhysicsDebugRender::WIREFRAME : PhysicsDebugRender::NONE;
		}

		if (parentMeshViewport->CollisionDirty)
		{
			TUsePointer<StaticMesh> staticMesh = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
			if (overlayPhysics)
				overlayPhysics->InvalidateMeshCollisionCache(staticMesh.GetRaw());
			parentMeshViewport->CollisionDirty = false;
		}

		FrameBuffer* renderFBO = gApplicationInfo->AppRenderingManager->RequestMainFrameBuffer().GetRaw();

		ImVec2 viewportSize = ImGui::GetContentRegionAvail();

		float availW = viewportSize.x;
		float availH = viewportSize.y;

		float texAspect = (float)renderFBO->GetWidth() / (float)renderFBO->GetHeight();
		float availAspect = availW / availH;

		ImVec2 imageSize;

		if (availAspect > texAspect) {
			imageSize.y = availH;
			imageSize.x = imageSize.y * texAspect;
		} else {
			imageSize.x = availW;
			imageSize.y = imageSize.x / texAspect;
		}

		GLuint texID = renderFBO->GetColorTexture()->GetID();
		ImTextureID imguiTex = (ImTextureID)(intptr_t)texID;

		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::Image(imguiTex, imageSize, ImVec2(0,1), ImVec2(1,0));

		bool hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + imageSize.x, pos.y + imageSize.y));
		if (hovered) {
			if (!gEditorAppContext->EditorScenesManager->IsInPIE()) {
				if (gEditorAppContext->EditorSceneCamera) {
					DynamicCast<EditorSceneCamera>(gEditorAppContext->EditorSceneCamera)->OnUpdate(deltaTime);
				}
			}
		}
	}
	EndPanel();
}
