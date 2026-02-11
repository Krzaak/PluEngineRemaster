//
// Created by Plutex on 1/1/26.
//

#include "EditorPanelManager.h"

#include "EditorPanel.h"

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
	const TOwningPointer<EditorPanel> newPanel = DynamicCast<EditorPanel>(mApplicationInfo->AppObjectManager->CreateObject(PanelClass));
	mPanels.PushBack(newPanel);
	newPanel->InitPanel(mApplicationInfo, this, mEditorAppContext);
	String panelInfo = "New panel attached: ";
	panelInfo += newPanel->GetClass()->TypeName;
	PLU_INFO(panelInfo.CStr());
	newPanel->OnShow();
	return newPanel;
}

void Plu::EditorPanelManager::Init()
{
}

void Plu::EditorPanelManager::OnUpdate(float deltaTime)
{
	for (TOwningPointer<EditorPanel> &panel: mPanels) {
		panel->OnUpdate(deltaTime);
	}
}

void Plu::EditorPanelManager::Shutdown()
{
}
