//
// Created by Plutex on 1/1/26.
//

#include "EditorPanel.h"

#include "EditorAppContext.h"
#include "EditorPanelManager.h"
#include "EditorWindows/EditorWindowsManager.h"
#include "PluEngine/Window/Window.h"
#include "PluEngine/Window/WindowManager.h"

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
	if (ImGui::BeginPopupContextItem()) {
		if (ImGui::BeginMenu("Move To Window")) {
			if (ImGui::MenuItem("New")) {
				mWindowIDToRender = mApplicationInfo->AppWindowsManager->GetWindowsAmount();
				mApplicationInfo->AppWindowsManager->AddWindow(WindowProperties(GetPanelName()));
				//mEditorAppContext->EditorWindowsManager->NewWindow();
			}
			ImGui::Separator();
			for (int i = 0; i < mApplicationInfo->AppWindowsManager->GetWindowsAmount(); i++) {
				if (ImGui::Selectable(String::FromInt(i).CStr())) {
					mWindowIDToRender = i;
				}
			}
			ImGui::EndMenu();
		}
		ImGui::EndPopup();
	}
	if (!mIsOpen) {
		mEditorPanelManager->ClosePanel(*this->GetEngineObjectHandle());
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
	mApplicationInfo->AppWindowsManager->GetObjectEventDispatcher()->Subscribe("ClosedWindow", [this](void* payload) {
		if (*static_cast<int*>(payload) == mWindowIDToRender) {
			mWindowIDToRender = 0;
			PLU_INFO("Panel going back to window 0");
		}
	});
}

int Plu::EditorPanel::GetWindowIDToRender()
{
	return mWindowIDToRender;
}
