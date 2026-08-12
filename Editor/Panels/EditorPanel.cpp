//
// Created by Plutex on 1/1/26.
//

#include "EditorPanel.h"

#include "EditorAppContext.h"
#include "EditorPanelManager.h"
#include "EditorWindows/EditorWindowsManager.h"
#include "EditorWindows/EditorWindowMoveMenu.h"
#include "PluEngine/Platform/Window.h"

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
		DrawMoveToWindowMenu(mWindowIDToRender, EEditorWindowKind::Dockspace, GetPanelName(),
			[this](UInt32 targetWindowID) {
				mEditorPanelManager->MovePanelToWindow(*this->GetEngineObjectHandle(), targetWindowID);
			});
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
	mWindowDispatcher = mApplicationInfo->AppWindow->GetObjectEventDispatcher();
	mWindowClosedHandle = mWindowDispatcher->Subscribe("WindowClosed", [this](void* payload) {
		if (*static_cast<UInt32*>(payload) == mWindowIDToRender) {
			mWindowIDToRender = 0;
			PLU_INFO("Panel going back to window 0");
		}
	});
}

Plu::EditorPanel::~EditorPanel()
{
	// The window (and its dispatcher) may be gone already during shutdown — TUsePointer goes
	// null-like then and there is nothing to unsubscribe from.
	if (mWindowDispatcher && mWindowClosedHandle != 0) {
		mWindowDispatcher->Unsubscribe("WindowClosed", mWindowClosedHandle);
	}
}

UInt32 Plu::EditorPanel::GetWindowIDToRender() const
{
	return mWindowIDToRender;
}

void Plu::EditorPanel::SetWindowIDToRender(UInt32 windowID)
{
	mWindowIDToRender = windowID;
}
