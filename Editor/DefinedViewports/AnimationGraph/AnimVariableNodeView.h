//
// Created by Plutex on 7/24/26.
//

#ifndef PLUENGINE_ANIMVARIABLENODEVIEW_H
#define PLUENGINE_ANIMVARIABLENODEVIEW_H

#include "NodeGraph/INodeView.h"
#include "NodeGraph/NodeGraphEditor.h"
#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimVariableNode.h"
#include "PluEngine/NodeGraph/GraphNode.h"
#include "PluEngine/NodeGraph/NodePin.h"
#include <imgui-node-editor/imgui_node_editor.h>
#include "imgui.h"

namespace ImGuiNodeEditor = ax::NodeEditor;

namespace Plu
{
	// A variable node is drawn as a small coloured "capsule" (rounded pill tinted in the variable's
	// type colour) instead of the full DefaultNodeView frame with a title bar: just the variable name
	// and its single output pin. Reads-a-variable nodes are visually lighter than logic nodes.
	class AnimVariableNodeView : public INodeView
	{
		// Variable type colour (registration Vec3 is 0-255) as an ImGui 0-1 colour; grey when the
		// variable is unresolved or its type is no longer registered.
		static ImVec4 TypeColor(const AnimVariableNode* node)
		{
			if (node->Variable) {
				if (const AnimationGraphVariableFactory::VariableTypeInfo* info =
					AnimationGraphVariableFactory::GetVariableTypeInfo(node->Variable->TypeName)) {
					return ImVec4(info->Color.x / 255.0f, info->Color.y / 255.0f, info->Color.z / 255.0f, 1.0f);
				}
			}
			return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
		}

		// Output pin dot, drawn by hand so its centre is a known point (the link pivot lands there —
		// same trick as DefaultNodeView). Must run between BeginPin/EndPin.
		static void DrawOutputDot(const ImVec4& color)
		{
			constexpr float radius = 4.5f;
			const float height = ImGui::GetTextLineHeight();
			const ImVec2 origin = ImGui::GetCursorScreenPos();
			ImGui::Dummy(ImVec2(radius * 2.0f, height));
			const ImVec2 center(origin.x + radius, origin.y + height * 0.5f);
			ImGui::GetWindowDrawList()->AddCircleFilled(center, radius, ImGui::GetColorU32(color), 16);
			ImGuiNodeEditor::PinPivotRect(center, center);
		}

	public:
		void Draw(GraphNode* node, NodeGraphEditor& editor) override
		{
			auto* variableNode = static_cast<AnimVariableNode*>(node);
			const ImVec4 typeColor = TypeColor(variableNode);

			// Pill look: strong rounding, tight padding, background tinted in the type colour with a
			// solid same-colour border.
			ImGuiNodeEditor::PushStyleVar(ImGuiNodeEditor::StyleVar_NodeRounding, 14.0f);
			ImGuiNodeEditor::PushStyleVar(ImGuiNodeEditor::StyleVar_NodePadding, ImVec4(12.0f, 3.0f, 10.0f, 3.0f));
			ImGuiNodeEditor::PushStyleColor(ImGuiNodeEditor::StyleColor_NodeBg,
				ImVec4(typeColor.x, typeColor.y, typeColor.z, 0.25f));
			ImGuiNodeEditor::PushStyleColor(ImGuiNodeEditor::StyleColor_NodeBorder, typeColor);

			ImGuiNodeEditor::BeginNode(editor.NodeId(node));
			ImGui::PushID(node);
			ImGui::BeginGroup();

			ImGui::TextUnformatted(variableNode->GetDisplayName().CStr());

			// The single output pin ("Value"), dot to the right of the name.
			if (!node->OutputPins.IsEmpty()) {
				NodePin& pin = node->OutputPins[0];
				ImGui::SameLine(0.0f, 8.0f);
				ImGuiNodeEditor::BeginPin(editor.PinId(node->Uuid, pin.Name, EPinDirection::Output),
					ImGuiNodeEditor::PinKind::Output);
				DrawOutputDot(typeColor);
				ImGuiNodeEditor::EndPin();
			}

			ImGui::EndGroup();
			ImGui::PopID();
			ImGuiNodeEditor::EndNode();

			ImGuiNodeEditor::PopStyleColor(2);
			ImGuiNodeEditor::PopStyleVar(2);
		}
	};
}

#endif //PLUENGINE_ANIMVARIABLENODEVIEW_H
