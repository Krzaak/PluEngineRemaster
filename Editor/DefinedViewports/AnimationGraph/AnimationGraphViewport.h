//
// Created by Plutex on 7/20/26.
//

#ifndef PLUENGINE_ANIMATIONGRAPHVIEWPORT_H
#define PLUENGINE_ANIMATIONGRAPHVIEWPORT_H

#include "EditorViewports/IEditorViewport.h"
#include <imgui-node-editor/imgui_node_editor.h>
#include "NodeGraph/NodeGraphEditor.h"
#include "AnimationGraphViewport.generated.h"

namespace ImGuiNodeEditor = ax::NodeEditor;

namespace Plu
{
	// Viewport for AnimationGraph assets. Two panels:
	//   - AnimationGraphViewportPanel: the node canvas, driven by the reusable NodeGraphEditor.
	//   - AnimationGraphDetailsPanel : reflected properties of the selected node.
	//
	// The shared editing state (the NodeGraphEditor: selection, id maps, layout) lives here so both
	// panels reach it. Node positions are editor-owned and persisted to a sidecar next to the asset,
	// never into the runtime .pluasset.
	PLU_CLASS()
	class AnimationGraphViewport : public IEditorViewport
	{
		REFLECTION_BODY_ANIMATIONGRAPHVIEWPORT()
	private:
		ImGuiNodeEditor::EditorContext* mNodeEditorContext = nullptr;
		NodeGraphEditor mNodeGraphEditor;

		[[nodiscard]] StringW GetLayoutPath();
	public:
		AnimationGraphViewport() = default;
		~AnimationGraphViewport() override = default;

		void OnClosed() override;
		void OnOpened() override;
		void OnPanelRegister() override;
		void OnUpdate(float deltaTime) override;

		ImGuiNodeEditor::EditorContext* GetNodeEditorContext() const;
		NodeGraphEditor& GetNodeGraphEditor() { return mNodeGraphEditor; }
	};
}

#endif //PLUENGINE_ANIMATIONGRAPHVIEWPORT_H
