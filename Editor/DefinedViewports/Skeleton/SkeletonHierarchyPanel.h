//
// Created by Plutex on 7/6/26.
//

#ifndef PLUENGINE_SKELETONHIERARCHYPANEL_H
#define PLUENGINE_SKELETONHIERARCHYPANEL_H
#include "EditorViewports/IEditorPanel.h"
#include "SkeletonHierarchyPanel.generated.h"
#include "PluEngine/Core.h"
#include "String/String.h"

namespace Plu
{
	struct SkeletonNode;
	struct Skeleton;

	// Reusable bone/node hierarchy tree. Deliberately viewport-agnostic: it owns its own view
	// state (visibility, node toggle, selection) and resolves the skeleton to draw from the
	// parent viewport's asset descriptor (a Skeleton asset directly, or a SkeletalMesh's
	// embedded skeleton). This lets both SkeletonViewport and SkeletalMeshViewport embed the
	// exact same panel without either one having to know about the other.
	PLU_CLASS()
	class SkeletonHierarchyPanel : public IEditorPanel
	{
		REFLECTION_BODY_SKELETONHIERARCHYPANEL()
	private:
		// Recursively draws one node. Non-bone nodes are skipped (their children rendered
		// in their place) unless ShowNodes is on.
		void DrawNodeTree(SkeletonNode* node);

		// Skeleton the parent viewport is editing: the asset itself when it's a Skeleton,
		// or MeshSkeleton when it's a SkeletalMesh. Null if neither / not loaded yet.
		Skeleton* ResolveSkeleton();
	public:
		SkeletonHierarchyPanel() = default;
		~SkeletonHierarchyPanel() override = default;

		// Set false to hide the panel (its docked tab is dropped and the space reclaimed).
		// Driven by the host viewport (e.g. SkeletalMeshViewport's "Show Hierarchy" toggle).
		bool Visible = true;

		// Show non-bone structural nodes in the tree in addition to bones.
		bool ShowNodes = false;

		// Name of the node highlighted here and in any peer preview panel (empty = none).
		String SelectedNodeName;

		String GetPanelName() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SKELETONHIERARCHYPANEL_H
