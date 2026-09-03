//
// Created by Plutex on 2026-06-26.
//

#ifndef PLUENGINE_LOADEDSHADERSPANEL_H
#define PLUENGINE_LOADEDSHADERSPANEL_H
#include "Panels/EditorPanel.h"
#include "LoadedShadersPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class LoadedShadersPanel : public EditorPanel
	{
		REFLECTION_BODY_LOADEDSHADERSPANEL()
	public:
		using EditorPanel::EditorPanel;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	};
}

#endif //PLUENGINE_LOADEDSHADERSPANEL_H
