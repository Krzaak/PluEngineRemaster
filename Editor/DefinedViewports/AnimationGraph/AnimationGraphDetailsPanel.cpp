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

namespace
{
	// First "NewVariable"/"NewVariable1"/... not already used by a variable in this graph.
	// Variables are addressed by Name (a "Get Variable" node references one), so names must be unique.
	Plu::String MakeUniqueVariableName(const Plu::TUsePointer<Plu::AnimationGraph>& graph)
	{
		auto isTaken = [&](const Plu::String& candidate) {
			for (const auto& variable : graph->Variables)
				if (variable && variable->Name == candidate) return true;
			return false;
		};

		Plu::String candidate = "NewVariable";
		for (int suffix = 1; isTaken(candidate); ++suffix)
			candidate = Plu::String("NewVariable") + Plu::String::FromInt(suffix);
		return candidate;
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
		} else {
			NodeGraphEditor& editor = viewport->GetNodeGraphEditor();
			GraphNode* node = editor.HasSelection() ? animationGraph->FindNode(editor.SelectedNode()) : nullptr;
			if (!node) {
				// Nothing selected: edit the graph asset's own properties (target skeleton, …).
				ImGui::TextUnformatted("Animation Graph");
				ImGui::Separator();
				if (TypeSerializer<TypeInfo*>::EditorControl(AnimationGraph::GetStaticClass(), animationGraph.GetRaw())) {
					PanelChangedAsset();
				}

				ImGui::Separator();
				ImGui::TextUnformatted("Variables");

				// "Add Variable" opens a palette of every registered variable type — the keys of the
				// factory map (Integer/Float/Bool/String/Vec3, see EditorApp::LoadFactories). Adding a
				// type there is the only change needed for it to show up here.
				if (ImGui::Button("Add Variable")) {
					ImGui::OpenPopup("AddVariablePopup");
				}
				if (ImGui::BeginPopup("AddVariablePopup")) {
					for (const auto& entry : AnimationGraphVariableFactory::GetFactoryMap()) {
						if (ImGui::MenuItem(entry.first.CStr())) {
							TOwningPointer<IAnimationGraphVariable> variable =
								AnimationGraphVariableFactory::CreateVariable(entry.first);
							if (variable) {
								variable->Name = MakeUniqueVariableName(animationGraph);
								animationGraph->Variables.PushBack(variable);
								PanelChangedAsset();
							}
						}
					}
					ImGui::EndPopup();
				}

				// Deferred: removing inside the loop would invalidate the iteration.
				Int32 variableToRemove = -1;
				for (Int32 index = 0; index < static_cast<Int32>(animationGraph->Variables.Size()); ++index) {
					const TOwningPointer<IAnimationGraphVariable>& variable = animationGraph->Variables[index];
					// PushID keeps the row's widgets unique per variable regardless of Name.
					ImGui::PushID(variable.GetRaw());

					char nameBuffer[128];
					const UInt64 nameLength = variable->Name.Length() < sizeof(nameBuffer) - 1
						? variable->Name.Length() : sizeof(nameBuffer) - 1;
					memcpy(nameBuffer, variable->Name.CStr(), nameLength);
					nameBuffer[nameLength] = '\0';

					ImGui::SetNextItemWidth(140.0f);
					if (ImGui::InputText("##VariableName", nameBuffer, sizeof(nameBuffer))) {
						variable->Name = nameBuffer;
						PanelChangedAsset();
					}
					ImGui::SameLine();
					// Hidden label: the name already has its own field above.
					if (variable->DrawEditorControl("##VariableValue")) {
						PanelChangedAsset();
					}
					ImGui::SameLine();
					if (ImGui::Button("X")) {
						variableToRemove = index;
					}

					ImGui::PopID();
				}
				if (variableToRemove >= 0) {
					animationGraph->Variables.RemoveAt(variableToRemove);
					PanelChangedAsset();
				}

				ImGui::Separator();
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
