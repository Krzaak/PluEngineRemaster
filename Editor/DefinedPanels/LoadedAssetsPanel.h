//
// Created by Plutex on 2026-06-26.
//

#ifndef PLUENGINE_LOADEDASSETSPANEL_H
#define PLUENGINE_LOADEDASSETSPANEL_H
#include "Panels/EditorPanel.h"
#include "LoadedAssetsPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class LoadedAssetsPanel : public EditorPanel
	{
		REFLECTION_BODY_LOADEDASSETSPANEL()
	public:
		using EditorPanel::EditorPanel;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	};
}

#endif //PLUENGINE_LOADEDASSETSPANEL_H
