//
// Created by Plutex on 1/14/26.
//

#ifndef PLUENGINE_STATICMESHVIEWPORTPANEL_H
#define PLUENGINE_STATICMESHVIEWPORTPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "StaticMeshViewportPanel.generated.h"
#include "PluEngine/Core.h"

namespace Plu
{
	class PrimitiveRenderable;
	PLU_CLASS()
	class StaticMeshViewportPanel : public IEditorPanel
	{
		REFLECTION_BODY_STATICMESHVIEWPORTPANEL()
	public:
		StaticMeshViewportPanel() = default;
		~StaticMeshViewportPanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_STATICMESHDETAILSPANEL_H