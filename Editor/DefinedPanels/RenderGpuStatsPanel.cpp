//
// Created by Plutex on 2026-06-26.
//

#include "RenderGpuStatsPanel.h"

#include "imgui.h"
#include "Panels/EditorPanelManager.h"
#include "TextureViewerPanel.h"
#include "PluEngine/Application.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Renderer/GLFrameBuffer.h"
#include "UI/IconsFontAwesome7.h"

Plu::String Plu::RenderGpuStatsPanel::GetPanelName()
{
	return ICON_FA_DISPLAY " Render / GPU";
}

void Plu::RenderGpuStatsPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel()) {
		const float renderFPS = GetRenderThreadFPS();
		ImGui::Text("Render thread: %.1f FPS (%.2f ms)", renderFPS, renderFPS > 0.0f ? 1000.0f / renderFPS : 0.0f);
		const float mainFPS = GetMainThreadFPS();
		ImGui::TextDisabled("Main thread:   %.1f FPS (%.2f ms)", mainFPS, mainFPS > 0.0f ? 1000.0f / mainFPS : 0.0f);
		ImGui::Separator();

		if (!mApplicationInfo->AppRenderingManager) {
			ImGui::TextDisabled("No rendering manager available.");
			EndPanel();
			return;
		}

		ImGui::SeparatorText("Triple Buffers");
		ImGui::TextDisabled("Dropped = producer outran consumer | Reused = consumer outran producer");
		TUsePointer<RenderingManager> rendering = mApplicationInfo->AppRenderingManager;
		if (ImGui::BeginTable("##triplebuffers", 3,
		                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
			ImGui::TableSetupColumn("Buffer", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Dropped", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Reused", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("Scene (RenderSnapshot)");
			ImGui::TableNextColumn();
			ImGui::Text("%u", rendering->GetSnapshotDroppedCount());
			ImGui::TableNextColumn();
			ImGui::Text("%u", rendering->GetSnapshotReusedCount());

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("ImGui draw data");
			ImGui::TableNextColumn();
			ImGui::Text("%u", rendering->GetImGuiDroppedCount());
			ImGui::TableNextColumn();
			ImGui::Text("%u", rendering->GetImGuiReusedCount());

			ImGui::EndTable();
		}
		if (ImGui::Button(ICON_FA_ARROWS_ROTATE " Reset Counters")) {
			rendering->ResetTripleBufferTelemetry();
		}

		ImGui::SeparatorText("Main Framebuffer");
		TUsePointer<FrameBuffer> mainFb = mApplicationInfo->AppRenderingManager->RequestMainFrameBuffer();
		if (mainFb) {
			ImGui::Text("Size: %d x %d", mainFb->GetWidth(), mainFb->GetHeight());
			ImGui::Text("Color attachment: %s", mainFb->GetColorTexture() ? "yes" : "no");
			ImGui::Text("Depth attachment: %s", mainFb->GetDepthTexture() ? "yes" : "no");
			if (ImGui::Button(ICON_FA_IMAGE " View Main Framebuffer")) {
				TUsePointer<TextureViewerPanel> viewer = mEditorPanelManager->AddPanel<TextureViewerPanel>();
				viewer->FrameBufferToView = mainFb;
			}
		} else {
			ImGui::TextDisabled("Main framebuffer not available yet.");
		}
	}
	EndPanel();
}

void Plu::RenderGpuStatsPanel::OnHide()
{
}

void Plu::RenderGpuStatsPanel::OnShow()
{
	SetCanClose(true);
}
