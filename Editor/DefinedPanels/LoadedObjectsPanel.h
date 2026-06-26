//
// Created by Plutex on 2026-06-26.
//

#ifndef PLUENGINE_LOADEDOBJECTSPANEL_H
#define PLUENGINE_LOADEDOBJECTSPANEL_H
#include "Panels/EditorPanel.h"
#include "LoadedObjectsPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class LoadedObjectsPanel : public EditorPanel
	{
		REFLECTION_BODY_LOADEDOBJECTSPANEL()
	public:
		using EditorPanel::EditorPanel;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	};
}

#endif //PLUENGINE_LOADEDOBJECTSPANEL_H
