//
// Created by Plutex on 1/14/26.
//

#ifndef PLUENGINE_SCENEINSPECTORPANEL_H
#define PLUENGINE_SCENEINSPECTORPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "SceneObjectDetailsPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class SceneInspectorPanel : public IEditorPanel
	{
		REFLECTION_BODY_SCENEINSPECTORPANEL()
	public:
		SceneInspectorPanel() = default;
		~SceneInspectorPanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_STATICMESHDETAILSPANEL_H