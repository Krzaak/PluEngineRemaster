//
// Created by Plutex on 7/7/26.
//

#ifndef PLUENGINE_SKELETALMESHDETAILSPANEL_H
#define PLUENGINE_SKELETALMESHDETAILSPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "SkeletalMeshDetailsPanel.generated.h"

namespace Plu
{
	// Side panel of SkeletalMeshViewport: preview material picker, mesh/skeleton info, the bone
	// overlay toggle, and the animation picker (loops the chosen clip; no timeline / scrubbing).
	PLU_CLASS()
	class SkeletalMeshDetailsPanel : public IEditorPanel
	{
		REFLECTION_BODY_SKELETALMESHDETAILSPANEL()
	public:
		SkeletalMeshDetailsPanel() = default;
		~SkeletalMeshDetailsPanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SKELETALMESHDETAILSPANEL_H
