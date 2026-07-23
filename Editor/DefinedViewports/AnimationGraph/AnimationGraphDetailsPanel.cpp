//
// Created by Plutex on 7/20/26.
//

#include "AnimationGraphDetailsPanel.h"

#include "AnimationGraphViewport.h"
#include "PluEngine/Application.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimVariableNode.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/NodeGraph/GraphNode.h"
#include "PluEngine/Reflection/TypeTraits.h"

extern Plu::ApplicationInfo* gApplicationInfo;

namespace
{
	// Variable inspector: name field, coloured type, value control. Shared by a variable selected in
	// the Variables panel and by a selected variable node (its underlying variable). Returns true if
	// a field changed (caller dirties the asset).
	bool AnimGraphDrawVariableDetails(const Plu::TUsePointer<Plu::IAnimationGraphVariable>& variable)
	{
		using namespace Plu;
		bool changed = false;

		ImGui::TextUnformatted("Variable");
		ImGui::Separator();

		// Name field — the variable's identity across the graph. Uniqueness is enforced when renaming
		// from the Variables panel; here we accept whatever is typed (a live-edited name colliding
		// with another only matters once a node references it).
		char nameBuffer[128];
		const UInt64 nameLength = variable->Name.Length() < sizeof(nameBuffer) - 1
			? variable->Name.Length() : sizeof(nameBuffer) - 1;
		memcpy(nameBuffer, variable->Name.CStr(), nameLength);
		nameBuffer[nameLength] = '\0';

		ImGui::TextUnformatted("Name");
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::InputText("##VariableName", nameBuffer, sizeof(nameBuffer))) {
			variable->Name = nameBuffer;
			changed = true;
		}

		// "Type:" label, then the type name painted in that type's registered colour.
		ImGui::TextUnformatted("Type:");
		ImGui::SameLine();
		ImVec4 typeColor(0.5f, 0.5f, 0.5f, 1.0f);
		if (const AnimationGraphVariableFactory::VariableTypeInfo* typeInfo =
			AnimationGraphVariableFactory::GetVariableTypeInfo(variable->TypeName)) {
			typeColor = ImVec4(typeInfo->Color.x / 255.0f, typeInfo->Color.y / 255.0f,
				typeInfo->Color.z / 255.0f, 1.0f);
		}
		ImGui::TextColored(typeColor, "%s", variable->TypeName.CStr());

		ImGui::TextUnformatted("Value");
		ImGui::SetNextItemWidth(-FLT_MIN);
		// Hidden label: the name has its own field above.
		if (variable->DrawEditorControl("##VariableValue")) {
			changed = true;
		}
		return changed;
	}
}

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
			EndPanel();
			return;
		}

		NodeGraphEditor& editor = viewport->GetNodeGraphEditor();
		GraphNode* node = editor.HasSelection() ? animationGraph->FindNode(editor.SelectedNode()) : nullptr;

		// What the inspector shows, in priority order:
		//  1. A variable selected in the Variables panel (it "stole" the inspector from the canvas —
		//     see AnimationGraphViewport::UpdateInspectorSelection), or
		//  2. a selected variable node → its underlying variable (so you edit the variable, not the
		//     node's raw VariableName key), or
		//  3. the selected node's own properties, or
		//  4. the graph asset's properties.
		TUsePointer<IAnimationGraphVariable> variable = viewport->GetSelectedVariable();
		if (!variable.IsValid()) {
			if (auto* variableNode = dynamic_cast<AnimVariableNode*>(node)) {
				variable = variableNode->Variable;
			}
		}

		if (variable.IsValid()) {
			if (AnimGraphDrawVariableDetails(variable)) {
				PanelChangedAsset();
			}
		} else if (node) {
			ImGui::TextUnformatted(node->GetDisplayName().CStr());
			ImGui::Separator();
			// One line renders every PLU_PROPERTY of the concrete node type; true = a field changed.
			if (TypeSerializer<TypeInfo*>::EditorControl(node->GetClass(), node)) {
				PanelChangedAsset();
			}
		} else {
			// Nothing selected: edit the graph asset's own properties (target skeleton, …).
			ImGui::TextUnformatted("Animation Graph");
			ImGui::Separator();
			if (TypeSerializer<TypeInfo*>::EditorControl(AnimationGraph::GetStaticClass(), animationGraph.GetRaw())) {
				PanelChangedAsset();
			}
			ImGui::Separator();
			ImGui::TextDisabled("Select a node or a variable to edit it");
		}
	}
	EndPanel();
}
