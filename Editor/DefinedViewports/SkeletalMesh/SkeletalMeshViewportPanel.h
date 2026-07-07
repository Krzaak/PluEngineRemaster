//
// Created by Plutex on 7/7/26.
//

#ifndef PLUENGINE_SKELETALMESHVIEWPORTPANEL_H
#define PLUENGINE_SKELETALMESHVIEWPORTPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "SkeletalMeshViewportPanel.generated.h"
#include "PluEngine/Core.h"

namespace Plu
{
	// The render panel of SkeletalMeshViewport: shows the overlay-scene FBO (like
	// StaticMeshViewportPanel), advances animation playback each frame, and optionally draws the
	// animated bone skeleton as an ImGui overlay projected with the editor camera.
	PLU_CLASS()
	class SkeletalMeshViewportPanel : public IEditorPanel
	{
		REFLECTION_BODY_SKELETALMESHVIEWPORTPANEL()
	public:
		SkeletalMeshViewportPanel() = default;
		~SkeletalMeshViewportPanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SKELETALMESHVIEWPORTPANEL_H
