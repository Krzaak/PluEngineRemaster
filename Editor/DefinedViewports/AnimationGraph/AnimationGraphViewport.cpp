//
// Created by Plutex on 7/20/26.
//

#include "AnimationGraphViewport.h"

#include "AnimationGraphViewportPanel.h"
#include "AnimationGraphDetailsPanel.h"
#include "AnimBlendNodeView.h"
#include "PluEngine/Assets/AssetDescriptor.h"

namespace Plu
{
	// One shared, stateless custom view instance for every blend node on every graph.
	static AnimBlendNodeView sAnimBlendNodeView;
}

Plu::StringW Plu::AnimationGraphViewport::GetLayoutPath()
{
	// Editor-owned layout sidecar next to the asset (node positions never enter the .pluasset).
	return (GetAssetDescriptor()->AssetPath.ToString() + ".layout.json").ToWide();
}

void Plu::AnimationGraphViewport::OnClosed()
{
	mNodeGraphEditor.SaveLayout(GetLayoutPath());
	ImGuiNodeEditor::DestroyEditor(mNodeEditorContext);
}

void Plu::AnimationGraphViewport::OnOpened()
{
	ImGuiNodeEditor::Config config;
	config.SettingsFile = nullptr; // node positions are owned by NodeGraphEditor's sidecar, not ed
	mNodeEditorContext = ImGuiNodeEditor::CreateEditor(&config);

	// Register domain-specific node visuals ("smaczki"); everything else uses the default view.
	mNodeGraphEditor.Registry().Register("AnimBlendNode", &sAnimBlendNodeView);

	mNodeGraphEditor.LoadLayout(GetLayoutPath());

	// Canvas first so it takes the larger (left) region; details dock to its right.
	AddPanel(AnimationGraphViewportPanel::GetStaticClass(), false);
	AddPanel(AnimationGraphDetailsPanel::GetStaticClass(), false);
}

void Plu::AnimationGraphViewport::OnPanelRegister()
{
	AnimationGraphViewportPanel* viewportPanel = GetPanelSlow<AnimationGraphViewportPanel>();
	AnimationGraphDetailsPanel* detailsPanel = GetPanelSlow<AnimationGraphDetailsPanel>();
	if (viewportPanel && detailsPanel) {
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		// Canvas on the left (~75%), details strip on the right.
		ImGuiID left, right;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Right, 0.25f, &right, &left);

		ImGui::DockBuilderDockWindow(viewportPanel->GetPanelTitle().CStr(), left);
		ImGui::DockBuilderDockWindow(detailsPanel->GetPanelTitle().CStr(), right);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::AnimationGraphViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdatePanels(deltaTime);
	}
	EndWindow();
}

ImGuiNodeEditor::EditorContext * Plu::AnimationGraphViewport::GetNodeEditorContext() const
{
	return mNodeEditorContext;
}
