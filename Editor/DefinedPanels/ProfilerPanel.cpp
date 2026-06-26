//
// Created by Plutex on 2026-06-20.
//

#include "ProfilerPanel.h"

#include "imgui.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Profiler.h"
#include "UI/IconsFontAwesome7.h"

Plu::String Plu::ProfilerPanel::GetPanelName()
{
	return ICON_FA_STOPWATCH " Profiler";
}

void Plu::ProfilerPanel::OnShow()
{
	SetCanClose(true);
}

void Plu::ProfilerPanel::OnHide()
{
}

void Plu::ProfilerPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel()) {
		const float mainFPS = GetMainThreadFPS();
		const float renderFPS = GetRenderThreadFPS();
		ImGui::Text("Main: %.1f FPS (%.2f ms)", mainFPS, mainFPS > 0.0f ? 1000.0f / mainFPS : 0.0f);
		ImGui::SameLine();
		ImGui::Text("| Render: %.1f FPS (%.2f ms)", renderFPS, renderFPS > 0.0f ? 1000.0f / renderFPS : 0.0f);
		ImGui::SameLine();
		if (ImGui::Button("Clear")) {
			Profiler::GetInstance()->Clear();
		}
		ImGui::Separator();

		GameHashMap<String, ProfilerEntry> snapshot = Profiler::GetInstance()->Snapshot();

		ImGuiTableFlags tableFlags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY;

		if (ImGui::BeginTable("##timings", 7, tableFlags)) {
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Last (ms)", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Avg (ms)", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Min (ms)", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Max (ms)", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("Calls", ImGuiTableColumnFlags_WidthFixed);
			ImGui::TableSetupColumn("History", ImGuiTableColumnFlags_WidthFixed, 140.0f);
			ImGui::TableHeadersRow();

			for (auto& pair : snapshot) {
				const String& name = pair.first;
				ProfilerEntry& entry = pair.second;

				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				ImGui::Text("%s", name.CStr());
				ImGui::TableNextColumn();
				ImGui::Text("%.3f", entry.LastMs);
				ImGui::TableNextColumn();
				ImGui::Text("%.3f", entry.AvgMs);
				ImGui::TableNextColumn();
				ImGui::Text("%.3f", entry.MinMs);
				ImGui::TableNextColumn();
				ImGui::Text("%.3f", entry.MaxMs);
				ImGui::TableNextColumn();
				ImGui::Text("%llu", static_cast<unsigned long long>(entry.TotalCalls));
				ImGui::TableNextColumn();
				// WriteIndex jako offset ring-buffera; skala 0..Max dla czytelności.
				String sparkId = "##spark_" + name;
				ImGui::PlotLines(sparkId.CStr(), entry.History, entry.SampleCount, entry.WriteIndex,
					nullptr, 0.0f, entry.MaxMs > 0.0f ? entry.MaxMs : 1.0f, ImVec2(130.0f, 30.0f));
			}
			ImGui::EndTable();
		}
	}
	EndPanel();
}
