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
	struct IAnimationGraphVariable;

	// Viewport for AnimationGraph assets. Three panels:
	//   - AnimationGraphViewportPanel : the node canvas, driven by the reusable NodeGraphEditor.
	//   - AnimationGraphVariablesPanel: the graph's variable list (add / rename / delete / select),
	//                                   modelled on the scene's Structure panel.
	//   - AnimationGraphDetailsPanel  : reflected properties of whatever is selected — the graph, a
	//                                   node, or a variable (name + value control), like scene Details.
	//
	// The shared editing state lives here so every panel reaches it: the NodeGraphEditor (canvas
	// selection, id maps, layout) and the "which variable is being inspected" selection. Node
	// positions are editor-owned and persisted to a sidecar next to the asset, never into the
	// runtime .pluasset.
	PLU_CLASS()
	class AnimationGraphViewport : public IEditorViewport
	{
		REFLECTION_BODY_ANIMATIONGRAPHVIEWPORT()
	private:
		ImGuiNodeEditor::EditorContext* mNodeEditorContext = nullptr;
		NodeGraphEditor mNodeGraphEditor;

		// The variable the Details panel inspects. A use-pointer so it clears itself when the
		// variable is deleted from the graph. Non-null "steals" the inspector from node/graph props.
		TUsePointer<IAnimationGraphVariable> mSelectedVariable;
		// Canvas selection seen last frame, to detect a *fresh* node click and hand the inspector
		// back from a selected variable to the node.
		bool    mLastNodeSelectionValid = false;
		PluUUID mLastSelectedNode = PluUUID(0);

		[[nodiscard]] StringW GetLayoutPath();

		// Clears mSelectedVariable when the user clicks a different node on the canvas, so a node
		// click always wins the inspector back. Run once per frame before the panels update.
		void UpdateInspectorSelection();
	public:
		AnimationGraphViewport() = default;
		~AnimationGraphViewport() override = default;

		void OnClosed() override;
		void OnOpened() override;
		void OnPanelRegister() override;
		void OnUpdate(float deltaTime) override;

		ImGuiNodeEditor::EditorContext* GetNodeEditorContext() const;
		NodeGraphEditor& GetNodeGraphEditor() { return mNodeGraphEditor; }

		[[nodiscard]] TUsePointer<IAnimationGraphVariable> GetSelectedVariable() const { return mSelectedVariable; }
		void SelectVariable(const TUsePointer<IAnimationGraphVariable>& variable) { mSelectedVariable = variable; }
	};
}

#endif //PLUENGINE_ANIMATIONGRAPHVIEWPORT_H
