//
// Created by Plutex on 7/7/26.
//

#ifndef PLUENGINE_ANIMATIONVIEWPORT_H
#define PLUENGINE_ANIMATIONVIEWPORT_H
#include "EditorViewports/IEditorViewport.h"
#include "AnimationViewport.generated.h"
#include "PluSTL_FWD.h"

namespace Plu
{
	// Viewport for standalone Animation assets. Shows a read-only timeline: one lane per
	// AnimationTrack, keyframes plotted along a time axis, with values/timestamps on hover.
	//
	// Everything is drawn with ImGui draw lists on the main thread (no GL / render snapshot),
	// same approach as SkeletonViewport. It owns the shared view state (zoom, selection) that
	// its single AnimationTimelinePanel consumes.
	PLU_CLASS()
	class AnimationViewport : public IEditorViewport
	{
		REFLECTION_BODY_ANIMATIONVIEWPORT()
	public:
		AnimationViewport() = default;
		virtual ~AnimationViewport() override = default;

		// --- Shared view state (pure preview — never marks the asset dirty) ---
		// Horizontal zoom of the timeline, in on-screen pixels per second.
		float PixelsPerSecond = 200.0f;

		// Set true to recompute PixelsPerSecond so the whole clip fits the panel width on the
		// next update (on open, or when the user clicks "Fit").
		bool NeedsFit = true;

		// Selected keyframe: track name + raw timestamp (ticks). Empty track = nothing selected.
		String SelectedTrack;
		double SelectedKeyTime = 0.0;
		bool HasSelection = false;

		void OnOpened() override;
		void OnClosed() override;
		void OnPanelRegister() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_ANIMATIONVIEWPORT_H
