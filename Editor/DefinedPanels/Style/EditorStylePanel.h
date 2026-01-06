//
// Created by Plutex on 1/5/26.
//

#ifndef PLUENGINE_EDITORSTYLEPANEL_H
#define PLUENGINE_EDITORSTYLEPANEL_H
#include "Panels/EditorPanel.h"
#include "EditorStylePanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class EditorStylePanel : public EditorPanel
	{
		REFLECTION_BODY_EDITORSTYLEPANEL()
	public:
		using EditorPanel::EditorPanel;

		void OnHide() override;
		void OnShow() override;
		void OnUpdate(float deltaTime) override;


	};
}

#endif //PLUENGINE_EDITORSTYLEPANEL_H