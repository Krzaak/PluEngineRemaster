//
// Created by Plutex on 1/14/26.
//

#ifndef PLUENGINE_SCENEVIEWPORTPANEL_H
#define PLUENGINE_SCENEVIEWPORTPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "SceneViewportPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class SceneViewportPanel : public IEditorPanel
	{
		REFLECTION_BODY_SCENEVIEWPORTPANEL()
	public:
		SceneViewportPanel() = default;
		virtual ~SceneViewportPanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_STATICMESHDETAILSPANEL_H