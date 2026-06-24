//
// Created by Plutex on 1/14/26.
//

#include "SceneViewportPanel.h"

#include "EditorAppContext.h"
#include "EditorSettings/EditorSettings.h"
#include "EditorSettings/EditorSettingsManager.h"
#include "Managers/Scene/EditorCamera.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Renderer/GLFrameBuffer.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/Window/Window.h"

extern Plu::ApplicationInfo* gApplicationInfo;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::String Plu::SceneViewportPanel::GetPanelName()
{
	return "Viewport";
}

void Plu::SceneViewportPanel::OnClosed()
{
}

void Plu::SceneViewportPanel::OnOpened()
{
	gApplicationInfo->AppScenesManager->UnloadOverlayScene();
}

void Plu::SceneViewportPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		TUsePointer<SceneInfo> scene = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
		if (scene)
		{
			ImVec2 viewportPos  = ImGui::GetCursorScreenPos();
			ImVec2 viewportSize = ImGui::GetContentRegionAvail();

			FrameBuffer* renderFBO = gApplicationInfo->AppRenderingManager->RequestMainFrameBuffer().GetRaw();

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
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 0.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
			if (gEditorAppContext->EditorScenesManager->IsInPIE()) {
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.7f, 0.5f, 0.2f, 1.0f));
			} else {
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
			}
			ImGui::BeginChild("Viewport", imageSize ,ImGuiChildFlags_Borders);
			ImVec2 pos = ImGui::GetCursorScreenPos();
			ImGui::Image(imguiTex, imageSize, ImVec2(0,1), ImVec2(1,0));
			if (ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + imageSize.x, pos.y + imageSize.y))) {
				if (!gEditorAppContext->EditorScenesManager->IsInPIE()) {
					if (DynamicCast<EditorSceneCamera>(gEditorAppContext->EditorSceneCamera)) {
						DynamicCast<EditorSceneCamera>(gEditorAppContext->EditorSceneCamera)->OnUpdate(deltaTime);
					}
				}
			}
			ImGui::EndChild();
			ImGui::PopStyleColor();
			ImGui::PopStyleVar(4);
		}
	}
	EndPanel();}
