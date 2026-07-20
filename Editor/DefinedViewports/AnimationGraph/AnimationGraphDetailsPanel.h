//
// Created by Plutex on 7/20/26.
//

#ifndef PLUENGINE_ANIMATIONGRAPHDETAILSPANEL_H
#define PLUENGINE_ANIMATIONGRAPHDETAILSPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "AnimationGraphDetailsPanel.generated.h"
#include "PluEngine/Core.h"

namespace Plu
{
	// Properties of the selected node, or of the graph itself when nothing is selected.
	// Boilerplate: prints text for now. Every mutation added here must call PanelChangedAsset()
	// (see Editor/CLAUDE.md — dirty marking).
	PLU_CLASS()
	class AnimationGraphDetailsPanel : public IEditorPanel
	{
		REFLECTION_BODY_ANIMATIONGRAPHDETAILSPANEL()
	public:
		AnimationGraphDetailsPanel() = default;
		~AnimationGraphDetailsPanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_ANIMATIONGRAPHDETAILSPANEL_H
