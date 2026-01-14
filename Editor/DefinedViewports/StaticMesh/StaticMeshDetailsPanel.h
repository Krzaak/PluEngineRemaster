//
// Created by Plutex on 1/14/26.
//

#ifndef PLUENGINE_STATICMESHDETAILSPANEL_H
#define PLUENGINE_STATICMESHDETAILSPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "StaticMeshDetailsPanel.generated.h"

namespace Plu
{
	PLU_CLASS()
	class StaticMeshDetailsPanel : public IEditorPanel
	{
		REFLECTION_BODY_STATICMESHDETAILSPANEL()
	public:
		StaticMeshDetailsPanel() = default;
		~StaticMeshDetailsPanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_STATICMESHDETAILSPANEL_H