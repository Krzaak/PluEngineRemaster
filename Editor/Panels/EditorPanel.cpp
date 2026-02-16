//
// Created by Plutex on 1/1/26.
//

#include "EditorPanel.h"

#include "EditorPanelManager.h"

void Plu::EditorPanel::SetCanClose(bool canClose)
{
	mCanClose = canClose;
}

bool Plu::EditorPanel::BeginPanel()
{
	if (!ImGui::GetWindowDockNode()) {
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.4f, 0.4f, 0.4f, 1));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
	}
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
	bool open = ImGui::Begin(GetPanelName().CStr(), mCanClose ? &mIsOpen : nullptr, flags);
	//TODO
	//if (ImGui::IsWindowHovered()) mpEditorState->ViewportManager->SetHoveredPanel(this);
	if (!mIsOpen) {
		mEditorPanelManager->ClosePanel(*mEditorPanelManager->GetPanelByClass(TClassPointer<EditorPanel>(GetClass()))->GetEngineObjectHandle());
		return false;
	}
	return open;
}

void Plu::EditorPanel::EndPanel()
{
	ImGui::End();
	if (!ImGui::GetWindowDockNode()) {
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
	}
}

Plu::EditorPanel::EditorPanel()
{
	mApplicationInfo = nullptr;
	mEditorPanelManager = nullptr;
}
void Plu::EditorPanel::InitPanel(ApplicationInfo *applicationInfo, EditorPanelManager* panelManager, EditorAppContext* editorAppContext)
{
	mApplicationInfo = applicationInfo;
	mEditorPanelManager = panelManager;
	mEditorAppContext = editorAppContext;
}
