//
// Created by Plutex on 1/1/26.
//

#include "EditorPanelManager.h"
#include "EditorPanel.h"
#include "DefinedPanels/ProjectLauncherPanel.h"
#include "EditorInterface.h"

Plu::EditorPanelManager::EditorPanelManager()
{
	mApplicationInfo = nullptr;
}

void Plu::EditorPanelManager::Init(ApplicationInfo *applicationInfo, EditorAppContext* editorAppContext)
{
	mApplicationInfo = applicationInfo;
	mEditorAppContext = editorAppContext;
}

Plu::EditorPanelManager::~EditorPanelManager()
{
}

Plu::TUsePointer<Plu::EditorPanel> Plu::EditorPanelManager::AddPanel(const TypeInfo *PanelClass)
{
	if (GetPanelByClass(const_cast<TypeInfo*>(PanelClass)))
	{
		return nullptr;
	}
	TUsePointer<EditorPanel> newPanelUser = mApplicationInfo->AppObjectManager->CreateObject(PanelClass);
	const TOwningPointer<EditorPanel> newPanel = mApplicationInfo->AppObjectManager->GetObjectAsOwner<EditorPanel>(newPanelUser->GetObjectHandle());
	mPanelsToRegister.PushBack(newPanel);
	return newPanel;
}

void Plu::EditorPanelManager::ClosePanel(EngineObjectHandle panel)
{
	TOwningPointer<EditorPanel>* panelPtr = mPanels.Find(mApplicationInfo->AppObjectManager->GetObjectAsOwner<EditorPanel>(panel));
	if (!panelPtr) return;
	mPanelsToDestroy.PushBack(*panelPtr);
}

Plu::TUsePointer<Plu::EditorPanel> Plu::EditorPanelManager::GetPanelByClass(TClassPointer<EditorPanel> panelClass)
{
	for (auto panel : mPanels)
	{
		if (panel->GetClass()->IsDerivedOfOrSame(panelClass))
		{
			return panel;
		}
	}
	// Also the ones added this frame but not yet initialised — to a caller asking "is a panel of
	// this class open?" they already are, and answering no leads to a duplicate being opened.
	for (auto panel : mPanelsToRegister)
	{
		if (panel->GetClass()->IsDerivedOfOrSame(panelClass))
		{
			return panel;
		}
	}
	return nullptr;
}

void Plu::EditorPanelManager::DockNewPanels(UInt32 windowID)
{
	// Docking runs against the *current* ImGui context, so a panel can only be docked while its
	// own window's frame is being built — hence the filter, and hence entries for other windows
	// stay queued until those windows get their turn this frame.
	const EditorWindowInfo* windowInfo = mEditorAppContext->EditorWindowsManager->GetWindowInfo(windowID);
	if (!windowInfo) return;

	DynamicArray<TOwningPointer<EditorPanel>> stillToDock;
	for (TOwningPointer<EditorPanel> &panel: mPanelsToDock) {
		if (panel->GetWindowIDToRender() != windowID) {
			stillToDock.PushBack(panel);
			continue;
		}
		ImGui::DockBuilderDockWindow(panel->GetPanelName().CStr(), windowInfo->DockspaceId);
		ImGui::DockBuilderFinish(windowInfo->DockspaceId);
		ImGui::SetWindowFocus(panel->GetPanelName().CStr());
	}
	mPanelsToDock = stillToDock;
}

void Plu::EditorPanelManager::ReturnPanelsFromWindow(UInt32 windowID)
{
	if (windowID == 0) return;
	for (TOwningPointer<EditorPanel> &panel: mPanels) {
		if (panel->GetWindowIDToRender() != windowID) continue;
		panel->SetWindowIDToRender(0);
		// Re-dock it in the main window: its dock node lived in the window that just died.
		mPanelsToDock.PushBack(panel);
	}
}

void Plu::EditorPanelManager::MovePanelToWindow(EngineObjectHandle panel, UInt32 targetWindowID)
{
	for (TOwningPointer<EditorPanel> &candidate: mPanels) {
		if (!(*candidate->GetEngineObjectHandle() == panel)) continue;
		if (candidate->GetWindowIDToRender() == targetWindowID) return;
		candidate->SetWindowIDToRender(targetWindowID);
		mEditorAppContext->EditorWindowsManager->RememberPanelWindow(candidate->GetClass()->TypeName, targetWindowID);
		// Its dock node belongs to the window it is leaving, so it needs a fresh one over there.
		if (!mPanelsToDock.Contains(candidate)) mPanelsToDock.PushBack(candidate);
		return;
	}
}

void Plu::EditorPanelManager::InitNewPanels()
{
	for (auto newPanel : mPanelsToRegister) {
		mPanels.PushBack(newPanel);
		newPanel->InitPanel(mApplicationInfo, this, mEditorAppContext);
		// A panel restored from the saved layout goes to the window that layout put it in; anything
		// else opens into the window the user is working in.
		UInt32 targetWindow = 0;
		if (!mEditorAppContext->EditorWindowsManager->TryTakePendingPanelWindow(newPanel->GetClass()->TypeName, targetWindow)) {
			if (!mEditorAppContext->EditorWindowsManager->TryGetActiveWindowID(targetWindow)) targetWindow = 0;
		}
		newPanel->SetWindowIDToRender(targetWindow);
		newPanel->OnShow();
		PLU_CORE_INFO("New Panel Opened, Class {}", newPanel->GetClass()->TypeName.CStr());
		mPanelsToDock.PushBack(newPanel);
	}
	mPanelsToRegister.Clear();
}

bool Plu::EditorPanelManager::AreTherePanelsToDock() const
{
	return !mPanelsToRegister.IsEmpty();
}

void Plu::EditorPanelManager::Init()
{
	AddPanel(ProjectLauncherPanel::GetStaticClass());
}

void Plu::EditorPanelManager::OnUpdate(float deltaTime, UInt32 windowID)
{
	for (TOwningPointer<EditorPanel> &panel: mPanels) {
		if (panel->GetWindowIDToRender() == windowID) {
			panel->OnUpdate(deltaTime);
		}
	}
	if (windowID != 0) return;
	for (TOwningPointer<EditorPanel> &panel: mPanelsToDestroy) {
		if (!mPanels.Remove(panel)) continue;
		panel->OnHide();
		mApplicationInfo->AppObjectManager->DestroyObject(*panel->GetEngineObjectHandle());
	}
	mPanelsToDestroy.Clear();
}

void Plu::EditorPanelManager::Shutdown()
{
}
