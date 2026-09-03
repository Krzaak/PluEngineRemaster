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
	struct AnimGraphInstance;

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

		// Name of the variable the Details panel inspects — a variable's stable identity (see
		// AnimGraphVariableStore, addressed by name). Empty = nothing selected. A String (not a
		// pointer) because the same name has to resolve against either the asset's defaults or a
		// live instance's store, whichever mInspectedInstance currently points at.
		String mSelectedVariableName;

		// Which value set GetSelectedVariable() resolves mSelectedVariableName against: null = the
		// asset's defaults, otherwise a live PIE instance (see AnimGraphInstance::GetLiveInstances).
		// Chosen via the Variables panel's instance combo; cleared in UpdateInspectorSelection() once
		// the instance drops out of the live registry (PIE ended).
		AnimGraphInstance* mInspectedInstance = nullptr;

		// Canvas selection seen last frame, to detect a *fresh* node click and hand the inspector
		// back from a selected variable to the node.
		bool    mLastNodeSelectionValid = false;
		PluUUID mLastSelectedNode = PluUUID(0);

		[[nodiscard]] StringW GetLayoutPath();

		// Clears mSelectedVariableName when the user clicks a different node on the canvas (a node
		// click always wins the inspector back), and drops mInspectedInstance once it's no longer in
		// the live registry (PIE ended under us). Run once per frame before the panels update.
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

		// Resolves mSelectedVariableName against mInspectedInstance's live store when one is chosen,
		// otherwise against the asset's defaults (AnimationGraph::FindVariable). Null when nothing is
		// selected or the name no longer resolves (e.g. the variable was deleted). Not const: it goes
		// through GetAssetDescriptor(), which isn't const either.
		[[nodiscard]] TUsePointer<IAnimationGraphVariable> GetSelectedVariable();
		void SelectVariable(const TUsePointer<IAnimationGraphVariable>& variable);
		// The raw name, for callers that just need to know which row to highlight (comparing
		// GetSelectedVariable()'s pointer doesn't work once it can resolve into a live instance's
		// store — a different clone than the asset's own variable of the same name).
		[[nodiscard]] const String& GetSelectedVariableName() const { return mSelectedVariableName; }

		// PIE instance picker (Variables panel combo): null = Defaults.
		[[nodiscard]] AnimGraphInstance* GetInspectedInstance() const { return mInspectedInstance; }
		void SetInspectedInstance(AnimGraphInstance* instance) { mInspectedInstance = instance; }
	};
}

#endif //PLUENGINE_ANIMATIONGRAPHVIEWPORT_H
