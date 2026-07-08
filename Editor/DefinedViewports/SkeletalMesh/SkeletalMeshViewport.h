//
// Created by Plutex on 7/7/26.
//

#ifndef PLUENGINE_SKELETALMESHVIEWPORT_H
#define PLUENGINE_SKELETALMESHVIEWPORT_H
#include "EditorViewports/IEditorViewport.h"
#include "SkeletalMeshViewport.generated.h"
#include "PluEngine/BasicEngineClasses/Components/SkeletalMeshComponent.h"
#include "PluEngine/GameObject/GameObject.h"
#include "Utils/GizmoUtils.h"

namespace Plu
{
	// GameObject spawned into the viewport's overlay scene to host the previewed skeletal mesh.
	// Mirrors StaticMeshViewport's EditorMeshObject: one SkeletalMeshComponent that the panels
	// drive (mesh/material/animation), rendered through the normal snapshot path.
	PLU_CLASS()
	class EditorSkeletalMeshObject : public GameObject
	{
		REFLECTION_BODY_EDITORSKELETALMESHOBJECT()
	public:
		EditorSkeletalMeshObject() = default;
		virtual ~EditorSkeletalMeshObject() override = default;

		void OnSetupComponents() override;

		TUsePointer<SkeletalMeshComponent> MeshComponent;
	};

	struct MaterialInfo;

	// Viewport for standalone SkeletalMesh assets. Renders the mesh through the overlay scene
	// (same FBO path as StaticMeshViewport) and adds skeletal-only preview state: a bone overlay
	// toggle and an animation picker (playback loops, no timeline / scrubbing).
	PLU_CLASS()
	class SkeletalMeshViewport : public IEditorViewport
	{
		REFLECTION_BODY_SKELETALMESHVIEWPORT()
	public:
		SkeletalMeshViewport() = default;
		virtual ~SkeletalMeshViewport() override = default;

		// --- Shared preview state (never marks the asset dirty — pure view settings) ---
		// Preview material applied to the mesh; not part of the asset.
		TUsePointer<MaterialInfo> Material;

		// Draw the animated bone skeleton as an ImGui overlay on top of the rendered mesh.
		bool ShowBones = false;

		// Expand the reusable bone/node hierarchy tree (same panel as SkeletonViewport) docked on
		// the left. Off by default; the details panel mirrors this into the panel's Visible flag.
		bool ShowHierarchy = false;

		// Set true to make the viewport panel fit the shared editor camera to the mesh bounds on
		// its next update (on open, or when the user clicks "Frame"). Stays pending until the mesh
		// has loaded vertices to measure, so async loads still get framed.
		bool NeedsFraming = true;

		// Bone-posing gizmo mode (view state; the viewport's overlay controls drive these).
		GizmoOperation GizmoOp = GizmoOperation::TRANSLATE;
		GizmoOperationSpace GizmoSpace = GizmoOperationSpace::WORLD;

		void OnOpened() override;
		void OnClosed() override;
		void OnPanelRegister() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SKELETALMESHVIEWPORT_H
