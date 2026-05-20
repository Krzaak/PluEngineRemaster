//
// Created by Plutex on 2026-03-05.
//

#include "StaticMeshViewportPanel.h"

#include "EditorAppContext.h"
#include "StaticMeshViewport.h"
#include "Managers/Scene/EditorCamera.h"
#include "Managers/Scene/EditorScenesManager.h"
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
	TUsePointer<SceneWorld> overlay = gEditorAppContext->EditorScenesManager->CreateOverlayWorld();
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
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();

		FrameBuffer* renderFBO = gApplicationInfo->AppRenderer->GetMainBuffer().GetRaw();

		float availW = viewportSize.x;
		float availH = viewportSize.y;

		float texAspect = (float)renderFBO->GetWidth() / (float)renderFBO->GetHeight();
		float availAspect = availW / availH;

		ImVec2 imageSize;

		// Dopasowanie bez zniekształcenia:
		if (availAspect > texAspect) {
			// Obszar UI jest bardziej poziomy → ograniczamy wysokość
			imageSize.y = availH;
			imageSize.x = imageSize.y * texAspect;
		} else {
			// Obszar UI jest bardziej pionowy → ograniczamy szerokość
			imageSize.x = availW;
			imageSize.y = imageSize.x / texAspect;
		}

		// Uzyskaj ID tekstury (ważne: to musi być zwykła tekstura, nie multisample!)
		GLuint texID = renderFBO->GetColorTexture()->GetID();

		// ImGui chce "ImTextureID"
		ImTextureID imguiTex = (ImTextureID)(intptr_t)texID;

		// Uwaga: OpenGL odwraca oś Y → dlatego UV są odwrotnie.
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::Image(imguiTex, imageSize, ImVec2(0,1), ImVec2(1,0));
		if (ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + imageSize.x, pos.y + imageSize.y))) {
			if (!gEditorAppContext->EditorScenesManager->IsInPIE()) {
				if (gEditorAppContext->EditorScenesManager->SceneCamera) {
					gEditorAppContext->EditorScenesManager->SceneCamera->OnUpdate(deltaTime);
				}
			}
		}
	}
	EndPanel();
}
