//
// Created by Plutex on 1/14/26.
//

#ifndef PLUENGINE_SCENESTRUCTUREPANEL_H
#define PLUENGINE_SCENESTRUCTUREPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "SceneStructurePanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class SceneStructurePanel : public IEditorPanel
	{
		REFLECTION_BODY_SCENESTRUCTUREPANEL()
	public:
		SceneStructurePanel() = default;
		~SceneStructurePanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_STATICMESHDETAILSPANEL_H