//
// Created by Plutex on 7/7/26.
//

#ifndef PLUENGINE_SKELETALMESHVIEWPORT_H
#define PLUENGINE_SKELETALMESHVIEWPORT_H
#include "EditorViewports/IEditorViewport.h"
#include "SkeletalMeshViewport.generated.h"
#include "PluEngine/BasicEngineClasses/Components/SkeletalMeshComponent.h"
#include "PluEngine/GameObject/GameObject.h"

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

		void OnOpened() override;
		void OnClosed() override;
		void OnPanelRegister() override;
		void OnUpdate(float deltaTime) override;
	};
}

#endif //PLUENGINE_SKELETALMESHVIEWPORT_H
