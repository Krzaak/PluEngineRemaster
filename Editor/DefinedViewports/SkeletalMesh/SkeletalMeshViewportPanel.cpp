//
// Created by Plutex on 7/7/26.
//

#include "SkeletalMeshViewportPanel.h"

#include <functional>
#include <glad/glad.h>
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "ImGuizmo/ImGuizmo.h"
#include "EditorAppContext.h"
#include "SkeletalMeshViewport.h"
#include "DefinedViewports/Skeleton/SkeletonHierarchyPanel.h"
#include "Managers/Scene/EditorCamera.h"
#include "PluEngine/Application.h"
#include "PluEngine/Physics/BoundingBox.h"
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

	// Re-fit the camera to the mesh whenever the viewport is (re)opened.
	parentViewport->NeedsFraming = true;
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
	// Highlights `selectedName` and, when `allowPick` is set, returns the bone/node whose joint is
	// clicked this frame (empty otherwise) so the caller can update the shared selection. For the
	// selected node it also writes the data the gizmo needs: its current world matrix (gizmo
	// target), the parent frame its local matrix lives in (componentWorld * parentGlobal), and its
	// base local matrix (bind/animated, before any override) — enough to fold a gizmo edit back
	// into a parent-space override.
	Plu::String DrawBoneOverlay(Plu::SkeletalMeshComponent* component, ImVec2 imagePos, ImVec2 imageSize,
	                            const Plu::String& selectedName, bool allowPick,
	                            Matrix4* outSelWorld, Matrix4* outSelParentFrame, Matrix4* outSelBaseLocal)
	{
		using namespace Plu;
		if (!component || !component->GetSkeletalMesh() || !component->GetSkeletalMesh()->MeshSkeleton) return String();
		SkeletonNode* root = component->GetSkeletalMesh()->MeshSkeleton->RootNode.GetRaw();
		if (!root) return String();

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
		const ImU32 selCol  = IM_COL32(90, 200, 255, 255);

		// Closest pickable element under the cursor (for click-to-select). A node can be grabbed
		// by its joint (a bit generous) or by the bone line running to it from its parent (tighter,
		// since lines are thin); whichever is nearer wins. Squared distances in pixels.
		const ImVec2 mouse = ImGui::GetMousePos();
		constexpr float kJointPickSq = 10.0f * 10.0f;
		constexpr float kLinePickSq  = 6.0f * 6.0f;
		float bestPickSq = kJointPickSq;
		String pickedName;

		// Perpendicular (clamped to the endpoints) squared distance from p to segment a→b.
		auto distToSegmentSq = [](ImVec2 p, ImVec2 a, ImVec2 b) -> float {
			const float abx = b.x - a.x, aby = b.y - a.y;
			const float denom = abx * abx + aby * aby;
			float t = denom > 0.0f ? ((p.x - a.x) * abx + (p.y - a.y) * aby) / denom : 0.0f;
			t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
			const float dx = p.x - (a.x + t * abx), dy = p.y - (a.y + t * aby);
			return dx * dx + dy * dy;
		};

		// Confine every stroke to the displayed image so bones can't spill over the panel chrome.
		drawList->PushClipRect(imagePos, ImVec2(imagePos.x + imageSize.x, imagePos.y + imageSize.y), true);

		// A bone's visual body is the segment(s) from its joint to its children, so the segment
		// parentNode→node belongs to parentNode: it's tinted when parentNode is selected, and
		// clicking it selects parentNode. The joint dot is the head of `node`, so it selects node.
		std::function<void(SkeletonNode*, SkeletonNode*, const Matrix4&, bool, ImVec2)> draw =
			[&](SkeletonNode* node, SkeletonNode* parentNode, const Matrix4& parentGlobal, bool hasParent, ImVec2 parentScreen) {
				if (!node) return;

				Matrix4 baseLocal = node->LocalMatrix;
				if (animation) {
					if (const AnimationTrack* track = animation->Tracks.Find(node->NodeName)) {
						const Vec3 loc = track->GetLocationAtTime(animTimeTicks);
						const Quaternion rot = track->GetRotationAtTime(animTimeTicks);
						const Vec3 scale = track->GetScaleAtTime(animTimeTicks);
						baseLocal = glm::translate(Matrix4(1.0f), loc) *
							glm::mat4_cast(rot) *
							glm::scale(Matrix4(1.0f), scale);
					}
				}

				// Temporary posing override (must match RenderSnapshotBuilder so overlay == mesh).
				Matrix4 local = baseLocal;
				if (!component->BoneLocalOverrides.IsEmpty()) {
					if (const Matrix4* ov = component->BoneLocalOverrides.Find(node->NodeName))
						local = (*ov) * baseLocal;
				}

				const Matrix4 global = parentGlobal * local;
				const Vec3 worldPos = Vec3((componentWorld * global)[3]);
				const bool isBone = dynamic_cast<SkeletonBone*>(node) != nullptr;
				const bool selected = !node->NodeName.IsEmpty() && node->NodeName == selectedName;
				const bool parentSelected = parentNode && !parentNode->NodeName.IsEmpty()
					&& parentNode->NodeName == selectedName;

				// Hand the caller what the gizmo needs for this bone (see function comment).
				if (selected) {
					if (outSelWorld) *outSelWorld = componentWorld * global;
					if (outSelParentFrame) *outSelParentFrame = componentWorld * parentGlobal;
					if (outSelBaseLocal) *outSelBaseLocal = baseLocal;
				}

				bool nextHasParent = hasParent;
				ImVec2 nextParent = parentScreen;
				SkeletonNode* nextParentNode = parentNode;

				ImVec2 screen;
				if (project(worldPos, screen)) {
					// Segment parentNode→node: tinted when parentNode (the bone it represents) is selected.
					if (hasParent) {
						const ImU32 lineCol = parentSelected ? selCol : (isBone ? boneCol : nodeCol);
						const float lineWidth = parentSelected ? (isBone ? 3.5f : 2.5f) : (isBone ? 2.0f : 1.0f);
						drawList->AddLine(parentScreen, screen, lineCol, lineWidth);
					}
					drawList->AddCircleFilled(screen, isBone ? 3.5f : 2.5f, isBone ? boneCol : nodeCol);
					nextHasParent = true;
					nextParent = screen;
					nextParentNode = node;

					if (allowPick) {
						// Joint hit → select this node (the joint is its head).
						if (!node->NodeName.IsEmpty()) {
							const float dx = mouse.x - screen.x, dy = mouse.y - screen.y;
							const float d2 = dx * dx + dy * dy;
							if (d2 < bestPickSq) { bestPickSq = d2; pickedName = node->NodeName; }
						}
						// Line hit → select parentNode (the bone that segment belongs to).
						if (hasParent && parentNode && !parentNode->NodeName.IsEmpty()) {
							const float ld2 = distToSegmentSq(mouse, parentScreen, screen);
							if (ld2 <= kLinePickSq && ld2 < bestPickSq) { bestPickSq = ld2; pickedName = parentNode->NodeName; }
						}
					}
				} else {
					nextHasParent = false;
				}

				for (const auto& child : node->Children)
					draw(child.GetRaw(), nextParentNode, global, nextHasParent, nextParent);
			};

		draw(root, nullptr, Matrix4(1.0f), false, ImVec2());

		drawList->PopClipRect();

		// Commit the pick only on a left click over the image, so hovering alone never reselects.
		if (allowPick && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !pickedName.IsEmpty())
			return pickedName;
		return String();
	}

	// Fits the shared editor camera to the mesh's bind-pose bounds (same helper the scene's
	// "Fit In View" uses). Returns false while the mesh has no vertices yet, so the caller can
	// keep the framing request pending across async loads.
	bool FrameCameraToMesh(Plu::EditorSkeletalMeshObject* meshObject, ImVec2 imageSize)
	{
		using namespace Plu;
		if (!meshObject || !meshObject->MeshComponent) return false;
		SkeletalMeshComponent* component = meshObject->MeshComponent.GetRaw();
		if (!component->GetSkeletalMesh()) return false;
		const SkeletalMeshData& meshData = component->GetSkeletalMesh()->MeshData;
		if (meshData.Vertices.IsEmpty()) return false;

		EditorSceneCamera* camera = gEditorAppContext->EditorSceneCamera.GetRaw();
		if (!camera) return false;

		DynamicArray<Vec3> points;
		points.Reserve(meshData.Vertices.Size());
		for (const auto& vertex : meshData.Vertices)
			points.PushBack(vertex.Position);

		BoundingBox box = CreateBoundingBox(points);
		const Vec3 newLoc = box.FitCamera(meshObject->GetObjectLocation(), camera->GetCameraRotation(),
		                                  Vec2(imageSize.x, imageSize.y), camera->GetCameraOptions()->FieldOfView);
		camera->SetCameraLocation(newLoc);
		return true;
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

		bool hovered = ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + imageSize.x, pos.y + imageSize.y));

		// Fit the camera to the mesh on request; keep it pending until the mesh has vertices.
		if (parentViewport && parentViewport->NeedsFraming && FrameCameraToMesh(meshObject.GetRaw(), imageSize))
			parentViewport->NeedsFraming = false;

		// Bone overlay is projected into the exact image rectangle with the same aspect the FBO
		// rendered at, so joints line up with the skinned mesh underneath. It highlights the bone
		// selected in the hierarchy panel and lets the user click a joint/line to (re)select it; a
		// gizmo on the selected bone (translate/rotate/scale, world/local) poses it temporarily.
		if (parentViewport && parentViewport->ShowBones && meshObject && meshObject->MeshComponent) {
			SkeletalMeshComponent* component = meshObject->MeshComponent.GetRaw();
			SkeletonHierarchyPanel* hierarchy = GetParentViewport()->GetPanelSlow<SkeletonHierarchyPanel>();
			const String selectedName = hierarchy ? hierarchy->SelectedNodeName : String();

			Matrix4 selWorld(1.0f), selParentFrame(1.0f), selBaseLocal(1.0f);
			// Don't let a click on the gizmo fall through to bone (re)selection.
			const bool allowPick = hovered && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing();
			const String picked = DrawBoneOverlay(component, pos, imageSize, selectedName, allowPick,
			                                      &selWorld, &selParentFrame, &selBaseLocal);
			if (hierarchy && !picked.IsEmpty())
				hierarchy->SelectedNodeName = picked;

			// Gizmo on the selected bone. The edited world matrix is converted back to a parent-space
			// override (override = newLocal * inverse(baseLocal)) — the same delta RenderSnapshotBuilder
			// applies — so the skinned mesh follows. Temporary only: BoneLocalOverrides is never saved.
			if (hierarchy && !hierarchy->SelectedNodeName.IsEmpty()) {
				Matrix4 view = RenderSnapshotBuilder::GetLastFrameViewMatrix();
				Matrix4 proj = RenderSnapshotBuilder::GetLastFrameProjectionMatrix();
				Matrix4 model = selWorld;

				ImGuizmo::SetOrthographic(false);
				ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
				ImGuizmo::SetRect(pos.x, pos.y, imageSize.x, imageSize.y);

				if (ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj),
				                         PluGizmoOperationToImGuizmoOperation(parentViewport->GizmoOp),
				                         PluGizmoModeToImGuizmoMode(parentViewport->GizmoSpace),
				                         glm::value_ptr(model))
					&& ImGuizmo::IsUsing()) {
					const Matrix4 newLocal = glm::inverse(selParentFrame) * model;
					const Matrix4 newOverride = newLocal * glm::inverse(selBaseLocal);

					Matrix4* slot = component->BoneLocalOverrides.Find(hierarchy->SelectedNodeName);
					if (!slot) {
						component->BoneLocalOverrides.Insert(hierarchy->SelectedNodeName, Matrix4(1.0f));
						slot = component->BoneLocalOverrides.Find(hierarchy->SelectedNodeName);
					}
					*slot = newOverride;
				}
			}

			// Overlay controls pinned top-left over the image (like the Skeleton/Animation viewports):
			// gizmo operation (move/rotate/scale) and space (world/local).
			auto toolButton = [](const char* label, bool active) -> bool {
				if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
				const bool clicked = ImGui::Button(label);
				if (active) ImGui::PopStyleColor();
				return clicked;
			};
			ImGui::SetCursorScreenPos(ImVec2(pos.x + 8.0f, pos.y + 8.0f));
			ImGui::BeginGroup();
			if (toolButton(ICON_FA_ARROWS_UP_DOWN_LEFT_RIGHT, parentViewport->GizmoOp == GizmoOperation::TRANSLATE))
				parentViewport->GizmoOp = GizmoOperation::TRANSLATE;
			ImGui::SameLine();
			if (toolButton(ICON_FA_ROTATE, parentViewport->GizmoOp == GizmoOperation::ROTATE))
				parentViewport->GizmoOp = GizmoOperation::ROTATE;
			ImGui::SameLine();
			if (toolButton(ICON_FA_MAXIMIZE, parentViewport->GizmoOp == GizmoOperation::SCALE))
				parentViewport->GizmoOp = GizmoOperation::SCALE;
			ImGui::SameLine(0.0f, 16.0f);
			const bool isWorld = parentViewport->GizmoSpace == GizmoOperationSpace::WORLD;
			// Single toggle showing (and switching) the current space.
			if (toolButton(isWorld ? ICON_FA_GLOBE " World" : ICON_FA_CUBE " Local", false))
				parentViewport->GizmoSpace = isWorld ? GizmoOperationSpace::LOCAL : GizmoOperationSpace::WORLD;

			// Reset just the selected bone — only shown once it's actually been posed.
			if (hierarchy && !hierarchy->SelectedNodeName.IsEmpty()
				&& component->BoneLocalOverrides.Find(hierarchy->SelectedNodeName)) {
				ImGui::SameLine(0.0f, 16.0f);
				if (ImGui::Button(ICON_FA_ROTATE_LEFT " Reset Transform"))
					component->BoneLocalOverrides.Remove(hierarchy->SelectedNodeName);
			}
			ImGui::EndGroup();
		}

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
