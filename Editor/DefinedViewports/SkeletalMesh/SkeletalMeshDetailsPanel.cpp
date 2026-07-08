//
// Created by Plutex on 7/7/26.
//

#include "SkeletalMeshDetailsPanel.h"

#include "EditorAppContext.h"
#include "SkeletalMeshViewport.h"
#include "DefinedViewports/Skeleton/SkeletonHierarchyPanel.h"
#include "PluEngine/Application.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/AssetTypes/Animation/SkeletalAnimation.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"
#include "PluEngine/AssetTypes/Skeleton/Skeleton.h"
#include "PluEngine/BasicEngineClasses/Components/SkeletalMeshComponent.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "UI/IconsFontAwesome7.h"

extern Plu::ApplicationInfo* gApplicationInfo;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::String Plu::SkeletalMeshDetailsPanel::GetPanelName()
{
	return "Properties";
}

void Plu::SkeletalMeshDetailsPanel::OnClosed()
{
}

void Plu::SkeletalMeshDetailsPanel::OnOpened()
{
}

void Plu::SkeletalMeshDetailsPanel::OnUpdate(float deltaTime)
{
	if (!BeginPanel())
	{
		EndPanel();
		return;
	}

	TUsePointer<SkeletalMeshViewport> parentViewport = DynamicCast<SkeletalMeshViewport>(GetParentViewport());
	TUsePointer<SkeletalMesh> skeletalMesh = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());

	TUsePointer<SceneWorld> overlay = gEditorAppContext->EditorScenesManager->GetCurrentWorld();
	TUsePointer<EditorSkeletalMeshObject> meshObject = overlay
		? DynamicCast<EditorSkeletalMeshObject>(overlay->GetGameObjectOfClass(EditorSkeletalMeshObject::GetStaticClass()))
		: nullptr;
	SkeletalMeshComponent* component = (meshObject && meshObject->MeshComponent) ? meshObject->MeshComponent.GetRaw() : nullptr;

	// --- Preview material (view-only: not part of the asset, so never marks it dirty) ---
	TUsePointer<MaterialInfo> before = parentViewport->Material;
	TypeSerializer<TUsePointer<MaterialInfo>>::EditorControl(&parentViewport->Material, "Material");
	if (before != parentViewport->Material && component)
		component->SetMaterial(parentViewport->Material);

	ImGui::Separator();

	// --- Mesh / skeleton info ---
	if (skeletalMesh)
	{
		ImGui::Text("Vertices: %lu", skeletalMesh->MeshData.Vertices.Size());
		ImGui::Text("Triangles: %lu", skeletalMesh->MeshData.Indices.Size() / 3);
		if (skeletalMesh->MeshSkeleton)
			ImGui::Text(ICON_FA_BONE " Skeleton: %s (%llu bones)",
				skeletalMesh->MeshSkeleton->SkeletonName.CStr(),
				(unsigned long long)skeletalMesh->MeshSkeleton->CountBones());
		else
			ImGui::TextDisabled(ICON_FA_BONE " No skeleton");
	}

	ImGui::Separator();

	// --- Overlay / panel toggles ---
	ImGui::Checkbox(ICON_FA_CIRCLE_NODES " Show Bones", &parentViewport->ShowBones);
	ImGui::SameLine();
	ImGui::Checkbox(ICON_FA_SITEMAP " Show Hierarchy", &parentViewport->ShowHierarchy);

	// Mirror the toggle into the shared hierarchy panel's visibility (single source of truth is
	// the viewport flag, so it persists while the panel is hidden). Same reusable tree as
	// SkeletonViewport — just embedded here on the left.
	if (SkeletonHierarchyPanel* hierarchyPanel = GetParentViewport()->GetPanelSlow<SkeletonHierarchyPanel>())
		hierarchyPanel->Visible = parentViewport->ShowHierarchy;

	// Re-fit the camera to the mesh bounds (also runs automatically when the viewport opens).
	if (ImGui::Button(ICON_FA_CROSSHAIRS " Frame"))
		parentViewport->NeedsFraming = true;

	// --- Temporary bone posing (Show Bones + move the selected bone's gizmo; never saved) ---
	if (parentViewport->ShowBones)
	{
		ImGui::TextDisabled("Select a bone, drag its gizmo to pose it (preview only).");
		if (component && !component->BoneLocalOverrides.IsEmpty())
		{
			ImGui::SameLine();
			if (ImGui::SmallButton(ICON_FA_ROTATE_LEFT " Reset Pose"))
				component->BoneLocalOverrides.Clear();
		}
	}

	ImGui::Separator();

	// --- Animation picker (loops the chosen clip; no timeline) ---
	if (ImGui::CollapsingHeader(ICON_FA_PERSON_RUNNING " Animation", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (!component)
		{
			ImGui::TextDisabled("Preview not ready.");
		}
		else
		{
			// Reuse the engine's standard asset picker for TUsePointer<Animation> (same control
			// the scene details panel draws for this property) — filtered, searchable, no bespoke
			// list to maintain. Auto-play the freshly picked clip from its start.
			if (TypeSerializer<TUsePointer<Animation>>::EditorControl(&component->AnimationToShow, "Animation"))
			{
				component->AnimationFrameToShow = 0;
				component->AnimationTimeTicks = 0.0f;
				component->IsPlaying = component->AnimationToShow.IsValid();
			}

			// Playback controls (no scrubber / timeline).
			if (component->AnimationToShow)
			{
				if (ImGui::Button(component->IsPlaying ? ICON_FA_PAUSE " Pause" : ICON_FA_PLAY " Play"))
					component->IsPlaying = !component->IsPlaying;
				ImGui::SameLine();
				if (ImGui::Button(ICON_FA_STOP " Reset"))
				{
					component->IsPlaying = false;
					component->AnimationFrameToShow = 0;
					component->AnimationTimeTicks = 0.0f;
				}
				ImGui::SameLine();
				ImGui::Checkbox("Loop", &component->LoopAnimation);

				ImGui::Text("%d frames @ %.1f fps",
					component->AnimationToShow->FramesAmount, component->AnimationToShow->FramesPerSecond);
			}
		}
	}

	EndPanel();
}
