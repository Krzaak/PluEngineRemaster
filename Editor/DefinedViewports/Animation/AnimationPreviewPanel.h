//
// Created by Plutex on 7/7/26.
//

#ifndef PLUENGINE_ANIMATIONPREVIEWPANEL_H
#define PLUENGINE_ANIMATIONPREVIEWPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "AnimationPreviewPanel.generated.h"
#include "PluEngine/Core.h"

namespace Plu
{
	// Live 3D preview of the animated skeleton. Samples every AnimationTrack at the viewport's
	// PlaybackTimeTicks, builds each node's world matrix (animated TRS where a track exists, else
	// bind-pose LocalMatrix), and projects the joints/bones to 2D with ImGui draw lists — the same
	// camera+projection approach as SkeletonViewportPanel, but posed each frame. No GL / overlay
	// scene, so it stays entirely on the main thread. Navigation uses the viewport's private
	// EditorSceneCamera (WASD fly / RMB look), gated on hover.
	PLU_CLASS()
	class AnimationPreviewPanel : public IEditorPanel
	{
		REFLECTION_BODY_ANIMATIONPREVIEWPANEL()
	public:
		AnimationPreviewPanel() = default;
		~AnimationPreviewPanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_ANIMATIONPREVIEWPANEL_H
