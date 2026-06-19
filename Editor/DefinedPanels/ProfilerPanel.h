//
// Created by Plutex on 2026-06-20.
//

#ifndef PLUENGINE_PROFILERPANEL_H
#define PLUENGINE_PROFILERPANEL_H
#include "Panels/EditorPanel.h"
#include "ProfilerPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class ProfilerPanel : public EditorPanel
	{
		REFLECTION_BODY_PROFILERPANEL()
	public:
		using EditorPanel::EditorPanel;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	};
}

#endif //PLUENGINE_PROFILERPANEL_H
