//
// Created by Plutex on 1/1/26.
//

#include "EditorPanel.h"

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
