//
// Created by Plutex on 7/20/26.
//

#include "AnimationGraphDetailsPanel.h"

#include "AnimationGraphViewport.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/NodeGraph/GraphNode.h"
#include "PluEngine/Reflection/TypeTraits.h"

extern Plu::ApplicationInfo* gApplicationInfo;

Plu::String Plu::AnimationGraphDetailsPanel::GetPanelName()
{
	return "Details";
}

void Plu::AnimationGraphDetailsPanel::OnClosed()
{
}

void Plu::AnimationGraphDetailsPanel::OnOpened()
{
}

void Plu::AnimationGraphDetailsPanel::OnUpdate(float deltaTime)
{
	if (BeginPanel())
	{
		TUsePointer<AnimationGraph> animationGraph =
			gApplicationInfo->AppAssetManager->GetAssetData(GetParentViewport()->GetAssetDescriptor());
		TUsePointer<AnimationGraphViewport> viewport = DynamicCast<AnimationGraphViewport>(GetParentViewport());

		if (!animationGraph || !viewport) {
			ImGui::TextDisabled("No AnimationGraph asset loaded");
		} else {
			NodeGraphEditor& editor = viewport->GetNodeGraphEditor();
			GraphNode* node = editor.HasSelection() ? animationGraph->FindNode(editor.SelectedNode()) : nullptr;
			if (!node) {
				ImGui::TextDisabled("Select a node to edit its properties");
			} else {
				ImGui::TextUnformatted(node->GetDisplayName().CStr());
				ImGui::Separator();
				// One line renders every PLU_PROPERTY of the concrete node type; true = a field changed.
				if (TypeSerializer<TypeInfo*>::EditorControl(node->GetClass(), node)) {
					PanelChangedAsset();
				}
			}
		}
	}
	EndPanel();
}
