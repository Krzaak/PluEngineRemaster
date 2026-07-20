//
// Created by Plutex on 7/20/26.
//

#ifndef PLUENGINE_ANIMATIONGRAPHVIEWPORTPANEL_H
#define PLUENGINE_ANIMATIONGRAPHVIEWPORTPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "AnimationGraphViewportPanel.generated.h"
#include "PluEngine/Core.h"

namespace Plu
{
	// The node canvas of an AnimationGraph — where nodes get placed, wired and selected.
	// Boilerplate: prints text for now.
	PLU_CLASS()
	class AnimationGraphViewportPanel : public IEditorPanel
	{
		REFLECTION_BODY_ANIMATIONGRAPHVIEWPORTPANEL()
	public:
		AnimationGraphViewportPanel() = default;
		~AnimationGraphViewportPanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_ANIMATIONGRAPHVIEWPORTPANEL_H
