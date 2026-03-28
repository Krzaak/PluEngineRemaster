//
// Created by Plutex on 2026-03-28.
//

#ifndef PLUENGINE_ASSETBROWSERPANEL_H
#define PLUENGINE_ASSETBROWSERPANEL_H

#include "Panels/EditorPanel.h"
#include "AssetBrowserPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class AssetBrowserPanel : public EditorPanel
	{
		REFLECTION_BODY_ASSETBROWSERPANEL()
	public:
		using EditorPanel::EditorPanel;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	};
}

#endif //PLUENGINE_ASSETBROWSERPANEL_H