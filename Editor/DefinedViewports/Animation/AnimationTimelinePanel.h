//
// Created by Plutex on 7/7/26.
//

#ifndef PLUENGINE_ANIMATIONTIMELINEPANEL_H
#define PLUENGINE_ANIMATIONTIMELINEPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "AnimationTimelinePanel.generated.h"
#include "PluEngine/Core.h"

namespace Plu
{
	// The single panel of AnimationViewport: draws the keyframe timeline (info bar + a
	// spreadsheet-style grid with a frozen time ruler across the top and a frozen track-name
	// column down the left). Pure ImGui draw-list rendering, main-thread only.
	PLU_CLASS()
	class AnimationTimelinePanel : public IEditorPanel
	{
		REFLECTION_BODY_ANIMATIONTIMELINEPANEL()
	public:
		AnimationTimelinePanel() = default;
		~AnimationTimelinePanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;

	private:
		// True while the user is dragging the playhead (scrubbing). Held across frames so the drag
		// keeps tracking the mouse even after it leaves the ruler strip. Pure UI state.
		bool mScrubbing = false;
	};
}

#endif //PLUENGINE_ANIMATIONTIMELINEPANEL_H
