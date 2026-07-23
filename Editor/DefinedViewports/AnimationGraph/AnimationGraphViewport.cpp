//
// Created by Plutex on 7/20/26.
//

#include "AnimationGraphViewport.h"

#include "AnimationGraphViewportPanel.h"
#include "AnimationGraphDetailsPanel.h"
#include "AnimationGraphVariablesPanel.h"
#include "AnimBlendNodeView.h"
#include "AnimVariableNodeView.h"
#include "PluEngine/Assets/AssetDescriptor.h"

namespace Plu
{
	// One shared, stateless custom view instance for every blend node on every graph.
	static AnimBlendNodeView sAnimBlendNodeView;
	// Likewise for the compact "capsule" variable-node view.
	static AnimVariableNodeView sAnimVariableNodeView;
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
	mNodeGraphEditor.Registry().Register("AnimVariableNode", &sAnimVariableNodeView);

	mNodeGraphEditor.LoadLayout(GetLayoutPath());

	// Canvas first so it takes the larger (left) region; the variables list and details dock into
	// the right-hand column (top and bottom).
	AddPanel(AnimationGraphViewportPanel::GetStaticClass(), false);
	AddPanel(AnimationGraphVariablesPanel::GetStaticClass(), false);
	AddPanel(AnimationGraphDetailsPanel::GetStaticClass(), false);
}

void Plu::AnimationGraphViewport::OnPanelRegister()
{
	AnimationGraphViewportPanel* viewportPanel = GetPanelSlow<AnimationGraphViewportPanel>();
	AnimationGraphVariablesPanel* variablesPanel = GetPanelSlow<AnimationGraphVariablesPanel>();
	AnimationGraphDetailsPanel* detailsPanel = GetPanelSlow<AnimationGraphDetailsPanel>();
	if (viewportPanel && variablesPanel && detailsPanel) {
		ImGuiID dockspaceID = GetWindowDockID();

		ImGui::DockBuilderRemoveNode(dockspaceID);
		ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetWindowSize());

		// Canvas on the left (~75%), a right-hand column split into Variables (top) and Details
		// (bottom) — mirrors the scene's Structure-over-Details arrangement.
		ImGuiID left, right;
		ImGui::DockBuilderSplitNode(dockspaceID, ImGuiDir_Right, 0.25f, &right, &left);
		ImGuiID rightTop, rightBottom;
		ImGui::DockBuilderSplitNode(right, ImGuiDir_Up, 0.5f, &rightTop, &rightBottom);

		ImGui::DockBuilderDockWindow(viewportPanel->GetPanelTitle().CStr(), left);
		ImGui::DockBuilderDockWindow(variablesPanel->GetPanelTitle().CStr(), rightTop);
		ImGui::DockBuilderDockWindow(detailsPanel->GetPanelTitle().CStr(), rightBottom);
		ImGui::DockBuilderFinish(dockspaceID);
	}
}

void Plu::AnimationGraphViewport::UpdateInspectorSelection()
{
	// A node click always wins the inspector back from a selected variable. We can't tell a click
	// apart from selection persisting across frames, so we clear the variable only when the canvas
	// selection *changes* to a (different) node — selecting a variable while a node stays highlighted
	// keeps showing the variable.
	const bool nodeSelected = mNodeGraphEditor.HasSelection();
	const PluUUID current = mNodeGraphEditor.SelectedNode();
	if (nodeSelected && (!mLastNodeSelectionValid || current != mLastSelectedNode)) {
		mSelectedVariable = nullptr;
	}
	mLastNodeSelectionValid = nodeSelected;
	mLastSelectedNode = current;
}

void Plu::AnimationGraphViewport::OnUpdate(float deltaTime)
{
	if (BeginWindow()) {
		UpdateInspectorSelection();
		UpdatePanels(deltaTime);
	}
	EndWindow();
}

ImGuiNodeEditor::EditorContext * Plu::AnimationGraphViewport::GetNodeEditorContext() const
{
	return mNodeEditorContext;
}
