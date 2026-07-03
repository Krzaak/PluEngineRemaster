//
// Created by Plutex on 2026-06-26.
//

#include "RenderGpuStatsPanel.h"

#include <algorithm>

#include "imgui.h"
#include "Panels/EditorPanelManager.h"
#include "TextureViewerPanel.h"
#include "PluEngine/Application.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Renderer/GLFrameBuffer.h"
#include "PluEngine/Renderer/RenderUsageStats.h"
#include "UI/IconsFontAwesome7.h"

Plu::String Plu::RenderGpuStatsPanel::GetPanelName()
{
	return ICON_FA_DISPLAY " Render / GPU";
}

void Plu::RenderGpuStatsPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel()) {
		if (ImGui::BeginTabBar("##rendergpu_tabs")) {
			if (ImGui::BeginTabItem(ICON_FA_GAUGE_HIGH " Overview")) {
				DrawOverviewTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem(ICON_FA_FIRE " Hottest Assets")) {
				DrawHottestAssetsTab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	EndPanel();
}

void Plu::RenderGpuStatsPanel::DrawOverviewTab()
{
	const float renderFPS = GetRenderThreadFPS();
	ImGui::Text("Render thread: %.1f FPS (%.2f ms)", renderFPS, renderFPS > 0.0f ? 1000.0f / renderFPS : 0.0f);
	const float mainFPS = GetMainThreadFPS();
	ImGui::TextDisabled("Main thread:   %.1f FPS (%.2f ms)", mainFPS, mainFPS > 0.0f ? 1000.0f / mainFPS : 0.0f);
	ImGui::Separator();

	if (!mApplicationInfo->AppRenderingManager) {
		ImGui::TextDisabled("No rendering manager available.");
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

namespace {
	// Wiersz do posortowania — kopia wpisu rejestru + jego UUID.
	struct HottestRow {
		UInt64 Uuid;
		Plu::AssetUsageEntry Entry;
	};

	// Zamienia snapshot rejestru na listę posortowaną malejąco po sumarycznym użyciu.
	DynamicArray<HottestRow> SortByTotalUses(const Plu::GameHashMap<UInt64, Plu::AssetUsageEntry>& stats)
	{
		DynamicArray<HottestRow> rows;
		for (const auto& pair : stats) {
			rows.PushBack(HottestRow{ pair.first, pair.second });
		}
		std::sort(rows.begin(), rows.end(), [](const HottestRow& a, const HottestRow& b) {
			if (a.Entry.TotalUses != b.Entry.TotalUses) return a.Entry.TotalUses > b.Entry.TotalUses;
			return a.Entry.LastFrameUses > b.Entry.LastFrameUses;
		});
		return rows;
	}
}

Plu::String Plu::RenderGpuStatsPanel::ResolveAssetName(UInt64 uuid)
{
	if (!mApplicationInfo->AppAssetManager) return "<no asset manager>";
	TUsePointer<AssetDescriptor> desc = mApplicationInfo->AppAssetManager->GetAssetDescriptor(PluUUID(uuid));
	if (!desc) return "<unknown>";
	if (!desc->AssetName.IsEmpty()) return desc->AssetName;
	// Fallback: nazwa pliku ze ścieżki assetu, gdy AssetName jest puste.
	String path = desc->AssetPath.ToString();
	if (!path.IsEmpty()) return path;
	return "<unnamed>";
}

void Plu::RenderGpuStatsPanel::DrawHottestAssetsTab()
{
	ImGui::TextDisabled("Ranked by usage in the scene pass. Shadow textures (CSM cascade maps) are not counted.");
	ImGui::TextDisabled("This frame = instances in the last frame | Total = accumulated since reset.");
	if (ImGui::Button(ICON_FA_ARROWS_ROTATE " Reset Usage Stats")) {
		RenderUsageStats::GetInstance()->Clear();
	}
	ImGui::Separator();

	const ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
		ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

	// --- Tekstury ---
	ImGui::SeparatorText(ICON_FA_IMAGE " Hottest Textures");
	{
		DynamicArray<HottestRow> rows = SortByTotalUses(RenderUsageStats::GetInstance()->GetTextureUsage());
		if (rows.Size() == 0) {
			ImGui::TextDisabled("No textures recorded yet.");
		} else if (ImGui::BeginTable("##hottest_textures", 3, tableFlags, ImVec2(0.0f, 180.0f))) {
			ImGui::TableSetupColumn("Texture", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("This frame", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();
			for (UInt32 i = 0; i < rows.Size(); i++) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%s", ResolveAssetName(rows[i].Uuid).CStr());
				ImGui::TableNextColumn();
				ImGui::Text("%u", rows[i].Entry.LastFrameUses);
				ImGui::TableNextColumn();
				ImGui::Text("%llu", static_cast<unsigned long long>(rows[i].Entry.TotalUses));
			}
			ImGui::EndTable();
		}
	}

	// --- Static Meshe ---
	ImGui::SeparatorText(ICON_FA_CUBES " Hottest Static Meshes");
	{
		DynamicArray<HottestRow> rows = SortByTotalUses(RenderUsageStats::GetInstance()->GetMeshUsage());
		if (rows.Size() == 0) {
			ImGui::TextDisabled("No static meshes recorded yet.");
		} else if (ImGui::BeginTable("##hottest_meshes", 3, tableFlags, ImVec2(0.0f, 180.0f))) {
			ImGui::TableSetupColumn("Static Mesh", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("This frame", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Total", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableHeadersRow();
			for (UInt32 i = 0; i < rows.Size(); i++) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%s", ResolveAssetName(rows[i].Uuid).CStr());
				ImGui::TableNextColumn();
				ImGui::Text("%u", rows[i].Entry.LastFrameUses);
				ImGui::TableNextColumn();
				ImGui::Text("%llu", static_cast<unsigned long long>(rows[i].Entry.TotalUses));
			}
			ImGui::EndTable();
		}
	}
}

void Plu::RenderGpuStatsPanel::OnHide()
{
}

void Plu::RenderGpuStatsPanel::OnShow()
{
	SetCanClose(true);
}
