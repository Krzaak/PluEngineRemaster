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

void Plu::EditorPanelManager::Init(ApplicationInfo *applicationInfo, EditorAppContext* editorAppContext, ImGuiID* assetDockspaceID)
{
	mApplicationInfo = applicationInfo;
	mEditorAppContext = editorAppContext;
	mAssetDockspaceID = assetDockspaceID;
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
	return nullptr;
}

void Plu::EditorPanelManager::DockNewPanels()
{
	for (TOwningPointer<EditorPanel> &panel: mPanelsToDock) {
		ImGui::DockBuilderDockWindow(panel->GetPanelName().CStr(), gDockspaceId);
		ImGui::DockBuilderFinish(gDockspaceId);
		ImGui::SetWindowFocus(panel->GetPanelName().CStr());
	}
	mPanelsToDock.Clear();
}

void Plu::EditorPanelManager::InitNewPanels()
{
	for (auto newPanel : mPanelsToRegister) {
		mPanels.PushBack(newPanel);
		newPanel->InitPanel(mApplicationInfo, this, mEditorAppContext);
		newPanel->OnShow();
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

void Plu::EditorPanelManager::OnUpdate(float deltaTime, int windowID)
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
