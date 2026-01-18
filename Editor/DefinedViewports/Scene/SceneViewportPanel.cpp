//
// Created by Plutex on 1/14/26.
//

#include "SceneViewportPanel.h"

#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Renderer/FrameBuffer.h"
#include "PluEngine/Renderer/Renderer.h"

extern Plu::ApplicationInfo* gApplicationInfo;

Plu::String Plu::SceneViewportPanel::GetPanelName()
{
	return "Viewport";
}

void Plu::SceneViewportPanel::OnClosed()
{
}

void Plu::SceneViewportPanel::OnOpened()
{
}

void Plu::SceneViewportPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		EditorAssetObject<SceneInfo>* scene = dynamic_cast<EditorAssetObject<SceneInfo>*>(GetParentViewport()->GetAssetObject().GetRaw());
		if (scene)
		{
			ImVec2 viewportSize = ImGui::GetContentRegionAvail();

			FrameBuffer* renderFBO = gApplicationInfo->AppRenderer->GetMainBuffer().GetRaw();

			float availW = viewportSize.x;
			float availH = viewportSize.y;

			float texAspect = (float)renderFBO->width() / (float)renderFBO->height();
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
			GLuint texID = renderFBO->colorTexture();

			// ImGui chce "ImTextureID"
			ImTextureID imguiTex = (ImTextureID)(intptr_t)texID;

			// Uwaga: OpenGL odwraca oś Y → dlatego UV są odwrotnie.
			ImGui::Image(imguiTex, imageSize, ImVec2(0,1), ImVec2(1,0));
		}
	}
	EndPanel();
}
