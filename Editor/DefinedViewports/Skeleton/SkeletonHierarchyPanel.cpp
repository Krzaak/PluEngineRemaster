//
// Created by Plutex on 7/6/26.
//

#include "SkeletonHierarchyPanel.h"

#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/Skeleton/Skeleton.h"
#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "UI/IconsFontAwesome7.h"

extern Plu::ApplicationInfo* gApplicationInfo;

Plu::String Plu::SkeletonHierarchyPanel::GetPanelName()
{
	return "Hierarchy";
}

void Plu::SkeletonHierarchyPanel::OnClosed()
{
}

void Plu::SkeletonHierarchyPanel::OnOpened()
{
}

Plu::Skeleton* Plu::SkeletonHierarchyPanel::ResolveSkeleton()
{
	TUsePointer<IAssetData> asset =
		gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
	if (!asset) return nullptr;

	// Standalone Skeleton asset — use it directly.
	if (Skeleton* skeleton = DynamicCast<Skeleton>(asset).GetRaw())
		return skeleton;

	// SkeletalMesh asset — draw its embedded skeleton.
	if (SkeletalMesh* mesh = DynamicCast<SkeletalMesh>(asset).GetRaw())
		return mesh->MeshSkeleton.GetRaw();

	return nullptr;
}

void Plu::SkeletonHierarchyPanel::DrawContextMenu(SkeletonNode* node)
{
	if (ImGui::BeginPopupContextItem()) {
		ImGui::Text("%s",node->NodeName.CStr());
		ImGui::Spacing();
		if (ImGui::Button("Add Attach Point")) {
			Skeleton* skeleton = ResolveSkeleton();
			TOwningPointer<SkeletonAttachPoint> newAttachPoint = CreateOwning<SkeletonAttachPoint>();
			newAttachPoint->ParentNodeName = node->NodeName;
			newAttachPoint->AttachPointName = node->NodeName + "AttachPoint";
			skeleton->AttachPoints.Insert(newAttachPoint->AttachPointName, newAttachPoint);
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
}

// True when node itself or any descendant would be shown with the current ShowNodes
// setting. Used to decide whether a tree node needs an expand arrow (Leaf otherwise).
static bool AnyVisibleDescendant(Plu::SkeletonNode* node, bool showNodes)
{
	for (const auto& child : node->Children) {
		if (!child) continue;
		bool childIsBone = dynamic_cast<Plu::SkeletonBone*>(child.GetRaw()) != nullptr;
		if (childIsBone || showNodes) return true;
		if (AnyVisibleDescendant(child.GetRaw(), showNodes)) return true;
	}
	return false;
}

void Plu::SkeletonHierarchyPanel::DrawNodeTree(SkeletonNode* node)
{
	if (!node) return;

	const bool isBone = dynamic_cast<SkeletonBone*>(node) != nullptr;
	const bool visible = isBone || ShowNodes;

	if (!visible) {
		// Hidden node: render its children where it would have been.
		for (const auto& child : node->Children)
			DrawNodeTree(child.GetRaw());
		return;
	}

	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen
		| ImGuiTreeNodeFlags_SpanAvailWidth;
	if (node->NodeName == SelectedNodeName)
		flags |= ImGuiTreeNodeFlags_Selected;
	if (!AnyVisibleDescendant(node, ShowNodes))
		flags |= ImGuiTreeNodeFlags_Leaf;

	const char* icon = isBone ? ICON_FA_BONE : ICON_FA_CIRCLE_NODES;
	// Bones bright, structural nodes dimmed so the two read differently at a glance.
	ImGui::PushStyleColor(ImGuiCol_Text, isBone ? ImVec4(0.95f, 0.85f, 0.4f, 1.0f)
	                                            : ImVec4(0.6f, 0.6f, 0.65f, 1.0f));
	ImGui::PushID(node);
	const bool open = ImGui::TreeNodeEx("##node", flags, "%s %s", icon, node->NodeName.CStr());
	ImGui::PopStyleColor();

	DrawContextMenu(node);

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
		SelectedNodeName = node->NodeName;

	if (open) {
		for (const auto& child : node->Children)
			DrawNodeTree(child.GetRaw());
		ImGui::TreePop();
	}
	ImGui::PopID();
}

void Plu::SkeletonHierarchyPanel::OnUpdate(float deltaTime)
{
	// Hidden: skip submission entirely so the docked tab disappears and its space is reclaimed.
	if (!Visible) return;

	if (BeginPanel())
	{
		Skeleton* skeleton = ResolveSkeleton();

		if (skeleton)
		{
			ImGui::TextDisabled("Skeleton: %s", skeleton->SkeletonName.CStr());
			ImGui::SameLine();
			ImGui::TextDisabled("(%llu bones)", (unsigned long long)skeleton->CountBones());
			ImGui::Separator();

			if (skeleton->RootNode)
			{
				// Deep rigs (e.g. a full human with finger joints) blow past the screen at the
				// default ~21px indent, so tighten it just for this tree.
				ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 10.0f);
				DrawNodeTree(skeleton->RootNode.GetRaw());
				ImGui::PopStyleVar();
			}
			else
				ImGui::TextDisabled("Skeleton has no nodes.");
		}
		else
		{
			ImGui::TextDisabled("No skeleton data.");
		}
	}
	EndPanel();
}
