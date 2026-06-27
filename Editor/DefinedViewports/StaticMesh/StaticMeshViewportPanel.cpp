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
#include "PluEngine/Renderer/PrimitiveRenderable.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"
#include "PluEngine/BasicEngineClasses/GameObjects/MeshObject.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Physics/StaticMeshCollisionBuilder.h"
#include "PluEngine/Physics/PhysicsWorld.h"
#include <glm/gtc/matrix_transform.hpp>

#include "PluEngine/Managers/RenderingManager.h"

extern Plu::ApplicationInfo* gApplicationInfo;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::String Plu::StaticMeshViewportPanel::GetPanelName()
{
	return "Viewport";
}

void Plu::StaticMeshViewportPanel::OnClosed()
{
	mCollisionRenderer = nullptr;
	mCachedCollisionShapes.Clear();
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

	mCollisionRenderer = CreateOwning<JoltWireframeRenderer>();

	RebuildCollisionShapes(staticMesh.GetRaw());
}

void Plu::StaticMeshViewportPanel::RebuildCollisionShapes(StaticMesh* mesh)
{
	mCachedCollisionShapes.Clear();
	if (!mesh) return;
	mCachedCollisionShapes = BuildCollisionShapesForMesh(mesh, Vec3(1.0f));
}

void Plu::StaticMeshViewportPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		TUsePointer<StaticMeshViewport> parentMeshViewport = DynamicCast<StaticMeshViewport>(GetParentViewport());

		if (parentMeshViewport->CollisionDirty)
		{
			TUsePointer<StaticMesh> staticMesh = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
			RebuildCollisionShapes(staticMesh.GetRaw());
			TUsePointer<SceneWorld> baseScene = gEditorAppContext->EditorScenesManager->GetBaseSceneWorld();
			if (baseScene && baseScene->GetPhysicsWorld())
				baseScene->GetPhysicsWorld()->InvalidateMeshCollisionCache(staticMesh.GetRaw());
			parentMeshViewport->CollisionDirty = false;
		}

		FrameBuffer* renderFBO = gApplicationInfo->AppRenderingManager->RequestMainFrameBuffer().GetRaw();

		// Render collision wireframe into the FBO before displaying it
		// if (parentMeshViewport->ShowCollision && mCollisionRenderer && !mCachedCollisionShapes.IsEmpty())
		// {
		// 	Matrix4 proj = gApplicationInfo->AppRenderer->GetProjectionMatrix();
		// 	Matrix4 view = gApplicationInfo->AppRenderer->GetViewMatrix();
		// 	Matrix4 viewProj = proj * view;
		//
		// 	renderFBO->Bind();
		// 	glViewport(0, 0, renderFBO->GetWidth(), renderFBO->GetHeight());
		//
		// 	glEnable(GL_DEPTH_TEST);
		// 	glDepthMask(GL_FALSE);
		//
		// 	mCollisionRenderer->BeginFrame();
		// 	for (const auto& entry : mCachedCollisionShapes)
		// 	{
		// 		glm::mat4 transform = glm::translate(glm::mat4(1.0f),
		// 			glm::vec3(entry.LocalOffset.x, entry.LocalOffset.y, entry.LocalOffset.z));
		// 		mCollisionRenderer->AddShape(entry.Shape, transform, glm::vec3(0.0f, 1.0f, 0.0f));
		// 	}
		// 	mCollisionRenderer->Render(viewProj);
		//
		// 	glDepthMask(GL_TRUE);
		// 	renderFBO->Unbind();
		// } TODO

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
