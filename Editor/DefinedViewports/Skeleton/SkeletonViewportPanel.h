//
// Created by Plutex on 7/6/26.
//

#ifndef PLUENGINE_SKELETONVIEWPORTPANEL_H
#define PLUENGINE_SKELETONVIEWPORTPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "SkeletonViewportPanel.generated.h"
#include "PluEngine/Core.h"

namespace Plu
{
	// Draws the skeleton preview by projecting bone/node positions to 2D and stroking
	// them with ImGui draw lists. No GL, no overlay scene — everything is main-thread.
	PLU_CLASS()
	class SkeletonViewportPanel : public IEditorPanel
	{
		REFLECTION_BODY_SKELETONVIEWPORTPANEL()
	public:
		SkeletonViewportPanel() = default;
		~SkeletonViewportPanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SKELETONVIEWPORTPANEL_H
