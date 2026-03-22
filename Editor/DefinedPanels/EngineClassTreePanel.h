//
// Created by Plutex on 1/18/26.
//

#ifndef PLUENGINE_ENGINECLASSTREEPANEL_H
#define PLUENGINE_ENGINECLASSTREEPANEL_H
#include "Panels/EditorPanel.h"
#include "PluEngine/Core.h"
#include "EngineClassTreePanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class EngineClassTreePanel : public EditorPanel
	{
		REFLECTION_BODY_ENGINECLASSTREEPANEL()
	public:
		using EditorPanel::EditorPanel;

		String GetPanelName() override;
		void OnUpdate(float deltaTime) override;
		void OnHide() override;
		void OnShow() override;
	};
}

#endif //PLUENGINE_ENGINECLASSTREEPANEL_H