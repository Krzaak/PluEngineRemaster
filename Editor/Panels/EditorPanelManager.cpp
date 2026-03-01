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
	const TOwningPointer<EditorPanel> newPanel = DynamicCast<EditorPanel>(mApplicationInfo->AppObjectManager->CreateObject(PanelClass));
	mPanels.PushBack(newPanel);
	newPanel->InitPanel(mApplicationInfo, this, mEditorAppContext);
	String panelInfo = "New panel attached: ";
	panelInfo += newPanel->GetClass()->TypeName;
	PLU_INFO(panelInfo.CStr());
	newPanel->OnShow();
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
	auto found = mPanels.FindIf([panelClass](TOwningPointer<EditorPanel> panel) -> bool {
		if (panel->GetClass()->IsDerivedOfOrSame(panelClass)) {
			return true;
		}
		return false;
	});
	return found ? *found : nullptr;
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
		mPanels.Remove(panel);
		panel->OnHide();
		mApplicationInfo->AppObjectManager->DestroyObject(*panel->GetEngineObjectHandle());
	}
	for (TOwningPointer<EditorPanel> &panel: mPanelsToRegister) {
		ImGui::DockBuilderDockWindow(panel->GetPanelName().CStr(), gDockspaceId);
		ImGui::DockBuilderFinish(gDockspaceId);
	}
	mPanelsToDestroy.Clear();
}

void Plu::EditorPanelManager::Shutdown()
{
}
