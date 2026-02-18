//
// Created by Plutex on 2026-02-18.
//

#ifndef PLUENGINE_INPUTVIEWERPANEL_H
#define PLUENGINE_INPUTVIEWERPANEL_H

#include "Panels/EditorPanel.h"
#include "InputViewerPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class InputViewerPanel : public EditorPanel
	{
		REFLECTION_BODY_INPUTVIEWERPANEL()
	public:
		using EditorPanel::EditorPanel;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	};
}

#endif //PLUENGINE_INPUTVIEWERPANEL_H