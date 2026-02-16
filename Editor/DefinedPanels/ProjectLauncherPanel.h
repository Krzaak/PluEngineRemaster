//
// Created by Plutex on 2026-02-16.
//

#ifndef PLUENGINE_PROJECTLAUNCHERPANEL_H
#define PLUENGINE_PROJECTLAUNCHERPANEL_H

#include "Panels/EditorPanel.h"
#include "ProjectLauncherPanel.generated.h"


namespace Plu
{
	PLU_CLASS()
	class ProjectLauncherPanel : public EditorPanel
	{
		REFLECTION_BODY_PROJECTLAUNCHERPANEL()
	public:
		using EditorPanel::EditorPanel;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	};
}

#endif //PLUENGINE_PROJECTLAUNCHERPANEL_H