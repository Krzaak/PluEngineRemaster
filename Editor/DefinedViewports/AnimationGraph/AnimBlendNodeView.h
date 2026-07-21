//
// Created by Plutex on 7/21/26.
//

#ifndef PLUENGINE_ANIMBLENDNODEVIEW_H
#define PLUENGINE_ANIMBLENDNODEVIEW_H

#include "NodeGraph/INodeView.h"
#include "NodeGraph/NodeGraphEditor.h"
#include "PluEngine/AssetTypes/AnimationGraph/Nodes/AnimBlendNode.h"
#include "imgui.h"

namespace Plu
{
	// The first visual "smaczek": AnimBlendNode draws an inline Alpha slider under its pins.
	// Everything else (header, pins, frame) is inherited from DefaultNodeView. Editing the slider
	// marks the asset modified through the editor callback.
	class AnimBlendNodeView : public DefaultNodeView
	{
	protected:
		void DrawBody(GraphNode* node, NodeGraphEditor& editor) override
		{
			auto* blend = static_cast<AnimBlendNode*>(node);
			ImGui::Dummy(ImVec2(0.0f, 2.0f));
			ImGui::SetNextItemWidth(120.0f);
			if (ImGui::SliderFloat("Alpha", &blend->Alpha, 0.0f, 1.0f)) {
				editor.MarkModified();
			}
		}
	};
}

#endif //PLUENGINE_ANIMBLENDNODEVIEW_H
