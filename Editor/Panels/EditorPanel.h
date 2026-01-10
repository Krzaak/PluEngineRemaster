//
// Created by Plutex on 1/1/26.
//

#ifndef PLUENGINE_EDITORPANEL_H
#define PLUENGINE_EDITORPANEL_H
#include "PluEngine/Objects/EngineObject.h"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "EditorPanel.generated.h"
#include "Pointers/TUsePointer.h"
#include "EditorPanelManager.h"

namespace Plu
{
	class EditorPanelManager;
	struct ApplicationInfo;

	PLU_CLASS(Abstract)
	class EditorPanel : public EngineObject
	{
		REFLECTION_BODY_EDITORPANEL()
	protected:
		ApplicationInfo* mApplicationInfo;
		EditorAppContext* mEditorAppContext;
		EditorPanelManager* mEditorPanelManager;
	public:
		EditorPanel();
		~EditorPanel() override = default;
		void InitPanel(ApplicationInfo *applicationInfo, EditorPanelManager* panelManager, EditorAppContext* editorAppContext);

		virtual void OnShow() = 0;
		virtual void OnUpdate(float deltaTime) = 0;
		virtual void OnHide() = 0;
	};
}

#endif //PLUENGINE_EDITORPANEL_H