//
// Created by Plutex on 7/6/26.
//

#ifndef PLUENGINE_SKELETONHIERARCHYPANEL_H
#define PLUENGINE_SKELETONHIERARCHYPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "SkeletonHierarchyPanel.generated.h"
#include "PluEngine/Core.h"

namespace Plu
{
	struct SkeletonNode;
	class SkeletonViewport;

	PLU_CLASS()
	class SkeletonHierarchyPanel : public IEditorPanel
	{
		REFLECTION_BODY_SKELETONHIERARCHYPANEL()
	private:
		// Recursively draws one node. Non-bone nodes are skipped (their children rendered
		// in their place) unless the viewport's ShowNodes is on.
		void DrawNodeTree(SkeletonNode* node, SkeletonViewport* viewport);
	public:
		SkeletonHierarchyPanel() = default;
		~SkeletonHierarchyPanel() override = default;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SKELETONHIERARCHYPANEL_H
