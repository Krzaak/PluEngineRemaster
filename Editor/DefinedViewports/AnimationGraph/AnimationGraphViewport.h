//
// Created by Plutex on 7/20/26.
//

#ifndef PLUENGINE_ANIMATIONGRAPHVIEWPORT_H
#define PLUENGINE_ANIMATIONGRAPHVIEWPORT_H
#include "EditorViewports/IEditorViewport.h"
#include "AnimationGraphViewport.generated.h"

namespace Plu
{
	// Viewport for AnimationGraph assets. Two panels:
	//   - AnimationGraphViewportPanel: the node canvas (graph editing surface).
	//   - AnimationGraphDetailsPanel : properties of the selected node / the graph itself.
	//
	// Boilerplate for now — both panels just print text. Shared editing state (selection, canvas
	// pan/zoom, ...) belongs here, on the viewport, so both panels can read it; see
	// AnimationViewport for the pattern.
	PLU_CLASS()
	class AnimationGraphViewport : public IEditorViewport
	{
		REFLECTION_BODY_ANIMATIONGRAPHVIEWPORT()
	public:
		AnimationGraphViewport() = default;
		~AnimationGraphViewport() override = default;

		void OnClosed() override;
		void OnOpened() override;
		void OnPanelRegister() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_ANIMATIONGRAPHVIEWPORT_H
