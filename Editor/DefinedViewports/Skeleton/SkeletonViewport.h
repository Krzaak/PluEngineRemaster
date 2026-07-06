//
// Created by Plutex on 7/6/26.
//

#ifndef PLUENGINE_SKELETONVIEWPORT_H
#define PLUENGINE_SKELETONVIEWPORT_H
#include "EditorViewports/IEditorViewport.h"
#include "SkeletonViewport.generated.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
	class EditorSceneCamera;

	// Viewport for standalone Skeleton assets. Owns the shared view state (camera, node
	// visibility, selection) consumed by its two panels: SkeletonHierarchyPanel (the
	// bone/node tree) and SkeletonViewportPanel (the 2D-projected 3D preview).
	//
	// The preview is drawn purely with ImGui draw lists (no overlay scene / FBO / render
	// snapshot), so this whole viewport lives on the main thread and touches no GL. It owns
	// a private EditorSceneCamera instance so navigation matches the scene viewport exactly
	// (same fly controls / directions) without sharing state with the real editor camera.
	PLU_CLASS()
	class SkeletonViewport : public IEditorViewport
	{
		REFLECTION_BODY_SKELETONVIEWPORT()
	public:
		SkeletonViewport() = default;
		virtual ~SkeletonViewport() override = default;

		// --- Shared view state (not asset data — never marks the asset dirty) ---
		// Non-bone nodes are hidden by default; the "Show Nodes" checkbox toggles this.
		bool ShowNodes = false;

		// Private navigation camera (same class/controls as the scene viewport camera).
		TOwningPointer<EditorSceneCamera> Camera;

		// Set true to make the viewport panel recompute framing from the skeleton bounds
		// on its next update (on open, or when the user clicks "Frame").
		bool NeedsFraming = true;

		// Name of the node highlighted in both panels (empty = none). Skeleton node names
		// are assimp-unique, so name matching is stable enough for editor selection.
		String SelectedNodeName;

		void OnInit() override;
		void OnClosed() override;
		void OnOpened() override;
		void OnPanelRegister() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SKELETONVIEWPORT_H
