//
// Created by Plutex on 1/1/26.
//

#ifndef PLUENGINE_ENGINESTATSPANEL_H
#define PLUENGINE_ENGINESTATSPANEL_H
#include "Panels/EditorPanel.h"
#include "EngineStatsPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class EngineStatsPanel : public EditorPanel
	{
		REFLECTION_BODY_ENGINESTATSPANEL()
	public:
		using EditorPanel::EditorPanel;

		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	};
}

#endif //PLUENGINE_ENGINESTATSPANEL_H