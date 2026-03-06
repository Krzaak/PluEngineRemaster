//
// Created by Plutex on 2026-03-05.
//

#include "StaticMeshViewportPanel.h"

#include "StaticMeshViewport.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Renderer/GLFrameBuffer.h"
#include "PluEngine/Renderer/PrimitiveRenderable.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/Input/InputManager.h"

extern Plu::ApplicationInfo* gApplicationInfo;

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
	gApplicationInfo->AppRenderer->ClearRenderables();
	TUsePointer<StaticMesh> mesh = gApplicationInfo->AppAssetManager->GetAssetByUUID(GetParentViewport()->GetAssetObject()->GetAssetInfoPtr()->Uuid);
	EngineObjectHandle rendr = gApplicationInfo->AppObjectManager->CreateObject<PrimitiveRenderable>(nullptr, mesh);
	mMeshRenderable = gApplicationInfo->AppObjectManager->GetObjectAsOwner<PrimitiveRenderable>(rendr);
	gApplicationInfo->AppRenderer->AddRenderable(mMeshRenderable.GetRaw());
}

void Plu::StaticMeshViewportPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		mMeshRenderable->SetMaterial(DynamicCast<StaticMeshViewport>(GetParentViewport())->Material);
		if (ImGui::IsWindowHovered()) {
			float cursorX = gApplicationInfo->AppInputManager->GetInputBackend()->GetMouse().scrollY;
			if (cursorX != 0) {
				mMeshRenderable->SetLocation(mMeshRenderable->GetRenderLocation() + Vec3(0,0,cursorX * 10));
			}
		}
		EditorAssetObject<StaticMesh>* staticMesh = dynamic_cast<EditorAssetObject<StaticMesh>*>(GetParentViewport()->GetAssetObject().GetRaw());
		if (staticMesh)
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
			ImGui::Image(imguiTex, imageSize, ImVec2(0,1), ImVec2(1,0));
		}
	}
	EndPanel();
}
