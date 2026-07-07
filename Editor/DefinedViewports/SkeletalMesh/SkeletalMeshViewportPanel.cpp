//
// Created by Plutex on 7/7/26.
//

#include "SkeletalMeshViewportPanel.h"

#include <functional>
#include <glad/glad.h>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "EditorAppContext.h"
#include "SkeletalMeshViewport.h"
#include "Managers/Scene/EditorCamera.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/Animation/SkeletalAnimation.h"
#include "PluEngine/AssetTypes/Material/Material.h"
#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"
#include "PluEngine/AssetTypes/Skeleton/Skeleton.h"
#include "PluEngine/BasicEngineClasses/Components/SkeletalMeshComponent.h"
#include "PluEngine/Managers/RenderingManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Renderer/GLFrameBuffer.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Renderer/RenderSnapshotBuilder.h"
#include "PluEngine/Scenes/SceneManager.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "UI/IconsFontAwesome7.h"

extern Plu::ApplicationInfo* gApplicationInfo;
extern Plu::EditorAppContext* gEditorAppContext;

Plu::String Plu::SkeletalMeshViewportPanel::GetPanelName()
{
	return "Viewport";
}

void Plu::SkeletalMeshViewportPanel::OnClosed()
{
}

void Plu::SkeletalMeshViewportPanel::OnOpened()
{
	PLU_INFO("Skeletal Mesh Viewport Opened!");
	gEditorAppContext->EditorScenesManager->CreateOverlayScene();
	TUsePointer<SceneWorld> overlay = gEditorAppContext->EditorScenesManager->GetCurrentWorld();
	TUsePointer<EditorSkeletalMeshObject> meshObject = overlay->SpawnGameObject(EditorSkeletalMeshObject::GetStaticClass());
	TUsePointer<SkeletalMesh> skeletalMesh = gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
	meshObject->MeshComponent->SetSkeletalMesh(skeletalMesh);
	TUsePointer<SkeletalMeshViewport> parentViewport = DynamicCast<SkeletalMeshViewport>(GetParentViewport());
	meshObject->MeshComponent->SetMaterial(parentViewport->Material);
}

namespace
{
	// Draws the animated bone skeleton over the rendered image. Recomputes the same pose the
	// render path builds (RenderSnapshotBuilder): per-node local = animated TRS if the current
	// animation has a track for it, else the bind-pose LocalMatrix; global = parent * local.
	//
	// Projection uses the *exact* view/projection the last frame was rendered with
	// (RenderSnapshotBuilder::GetLastFrame*Matrix), not a freshly rebuilt one. That guarantees
	// the joints land on the skinned mesh pixels regardless of FBO/window aspect: the displayed
	// texture and these matrices come from the same frame, and NDC maps linearly onto the whole
	// image rectangle. Everything is clipped to that rectangle so nothing bleeds outside.
	void DrawBoneOverlay(Plu::SkeletalMeshComponent* component, ImVec2 imagePos, ImVec2 imageSize)
	{
		using namespace Plu;
		if (!component || !component->GetSkeletalMesh() || !component->GetSkeletalMesh()->MeshSkeleton) return;
		SkeletonNode* root = component->GetSkeletalMesh()->MeshSkeleton->RootNode.GetRaw();
		if (!root) return;

		const Matrix4 viewProj =
			RenderSnapshotBuilder::GetLastFrameProjectionMatrix() * RenderSnapshotBuilder::GetLastFrameViewMatrix();
		const Matrix4 componentWorld = component->GetRenderMatrix();

		auto project = [&](const Vec3& world, ImVec2& out) -> bool {
			const Vec4 clip = viewProj * Vec4(world, 1.0f);
			if (clip.w <= 0.0001f) return false; // behind camera
			const Vec3 ndc = Vec3(clip) / clip.w;
			out.x = imagePos.x + (ndc.x * 0.5f + 0.5f) * imageSize.x;
			out.y = imagePos.y + (1.0f - (ndc.y * 0.5f + 0.5f)) * imageSize.y;
			return true;
		};

		TUsePointer<Animation> animation = component->AnimationToShow;
		double animTimeTicks = 0.0;
		if (animation) {
			animTimeTicks = component->IsPlaying
				? glm::clamp(static_cast<double>(component->AnimationTimeTicks), 0.0, static_cast<double>(animation->FramesAmount))
				: static_cast<double>(glm::clamp(component->AnimationFrameToShow, 0, animation->FramesAmount));
		}

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImU32 boneCol = IM_COL32(240, 216, 100, 255);
		const ImU32 nodeCol = IM_COL32(140, 140, 150, 200);

		// Confine every stroke to the displayed image so bones can't spill over the panel chrome.
		drawList->PushClipRect(imagePos, ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y), true);

		std::function<void(SkeletonNode*, const Matrix4&, bool, ImVec2)> draw =
			[&](SkeletonNode* node, const Matrix4& parentGlobal, bool hasParent, ImVec2 parentScreen) {
				if (!node) return;

				Matrix4 local = node->LocalMatrix;
				if (animation) {
					if (const AnimationTrack* track = animation->Tracks.Find(node->NodeName)) {
						const Vec3 loc = track->GetLocationAtTime(animTimeTicks);
						const Quaternion rot = track->GetRotationAtTime(animTimeTicks);
						const Vec3 scale = track->GetScaleAtTime(animTimeTicks);
						local = glm::translate(Matrix4(1.0f), loc) *
							glm::mat4_cast(rot) *
							glm::scale(Matrix4(1.0f), scale);
					}
				}

				const Matrix4 global = parentGlobal * local;
				const Vec3 worldPos = Vec3((componentWorld * global)[3]);
				const bool isBone = dynamic_cast<SkeletonBone*>(node) != nullptr;

				bool nextHasParent = hasParent;
				ImVec2 nextParent = parentScreen;

				ImVec2 screen;
				if (project(worldPos, screen)) {
					if (hasParent)
						drawList->AddLine(parentScreen, screen, isBone ? boneCol : nodeCol, isBone ? 2.0f : 1.0f);
					drawList->AddCircleFilled(screen, isBone ? 3.5f : 2.5f, isBone ? boneCol : nodeCol);
					nextHasParent = true;
					nextParent = screen;
				} else {
					nextHasParent = false;
				}

				for (const auto& child : node->Children)
					draw(child.GetRaw(), global, nextHasParent, nextParent);
			};

		draw(root, Matrix4(1.0f), false, ImVec2());

		drawList->PopClipRect();
	}
}

void Plu::SkeletalMeshViewportPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		TUsePointer<SkeletalMeshViewport> parentViewport = DynamicCast<SkeletalMeshViewport>(GetParentViewport());
		TUsePointer<SceneWorld> overlay = gEditorAppContext->EditorScenesManager->GetCurrentWorld();
		TUsePointer<EditorSkeletalMeshObject> meshObject = overlay
			? DynamicCast<EditorSkeletalMeshObject>(overlay->GetGameObjectOfClass(EditorSkeletalMeshObject::GetStaticClass()))
			: nullptr;

		// The overlay scene doesn't tick outside PIE, so advance animation playback ourselves.
		// SkeletalMeshComponent::OnUpdate no-ops when not playing, so calling it every frame is safe.
		if (meshObject && meshObject->MeshComponent)
			meshObject->MeshComponent->OnUpdate(deltaTime);

		FrameBuffer* renderFBO = gApplicationInfo->AppRenderingManager->RequestMainFrameBuffer().GetRaw();

		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		float availW = viewportSize.x;
		float availH = viewportSize.y;

		float texAspect = (float)renderFBO->GetWidth() / (float)renderFBO->GetHeight();
		float availAspect = availW / availH;

		ImVec2 imageSize;
		if (availAspect > texAspect) {
			imageSize.y = availH;
			imageSize.x = imageSize.y * texAspect;
		} else {
			imageSize.x = availW;
			imageSize.y = imageSize.x / texAspect;
		}

		GLuint texID = renderFBO->GetColorTexture()->GetID();
		ImTextureID imguiTex = (ImTextureID)(intptr_t)texID;

		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImGui::Image(imguiTex, imageSize, ImVec2(0,1), ImVec2(1,0));

		// Bone overlay is projected into the exact image rectangle with the same aspect the FBO
		// rendered at, so joints line up with the skinned mesh underneath.
		if (parentViewport && parentViewport->ShowBones && meshObject && meshObject->MeshComponent)
			DrawBoneOverlay(meshObject->MeshComponent.GetRaw(), pos, imageSize);

		bool hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + imageSize.x, pos.y + imageSize.y));
		if (hovered) {
			if (!gEditorAppContext->EditorScenesManager->IsInPIE()) {
				if (gEditorAppContext->EditorSceneCamera) {
					DynamicCast<EditorSceneCamera>(gEditorAppContext->EditorSceneCamera)->OnUpdate(deltaTime);
				}
			}
		}
	}
	EndPanel();
}
