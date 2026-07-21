//
// Created by Plutex on 7/21/26.
//

#ifndef PLUENGINE_NODEGRAPHEDITOR_H
#define PLUENGINE_NODEGRAPHEDITOR_H

#include "PluEngine/Core.h"
#include "PluEngine/PluUUID.h"
#include "PluEngine/NodeGraph/NodePin.h"
#include "PluEngine/NodeGraph/NodeLink.h"
#include "NodeGraph/NodeViewRegistry.h"
#include "HashMap/HashMapV2.h"
#include "String/String.h"
#include <imgui-node-editor/imgui_node_editor.h>
#include <functional>

namespace Plu
{
	struct NodeGraph;
	struct GraphNode;

	// Reusable, domain-agnostic driver for an imgui-node-editor canvas over any NodeGraph. Handles
	// drawing (via the NodeView registry), link create/delete with pin-type validation, an "Add Node"
	// palette built from reflection, node deletion, selection tracking, and editor-owned node layout
	// (positions live here + a sidecar file, never in the runtime asset).
	//
	// Must be driven between ed::Begin() and ed::End() on the current editor context (the owning
	// viewport sets the context and brackets the frame).
	class NodeGraphEditor
	{
	public:
		NodeGraphEditor() = default;

		// Draw + interact for one frame. onModified is invoked after any mutation so the caller can
		// dirty-mark the asset. Call between ed::Begin()/ed::End().
		void Draw(NodeGraph* graph, const std::function<void()>& onModified);

		NodeViewRegistry& Registry() { return mRegistry; }

		// Primary selected node, for the details panel.
		[[nodiscard]] bool HasSelection() const { return mHasSelection; }
		[[nodiscard]] const PluUUID& SelectedNode() const { return mSelectedNode; }

		// Session-stable ephemeral ids for imgui-node-editor. Used by node views while drawing.
		ax::NodeEditor::NodeId NodeId(const GraphNode* node);
		ax::NodeEditor::PinId  PinId(const PluUUID& nodeUuid, const String& pinName, EPinDirection direction);

		// Views (e.g. an inline param slider) call this after mutating a node.
		void MarkModified();

		// Editor-owned node layout persistence. Positions never enter the runtime .pluasset.
		void LoadLayout(const StringW& path);
		void SaveLayout(const StringW& path) const;

	private:
		void DrawNodes(NodeGraph* graph);
		void DrawLinks(NodeGraph* graph);
		void HandleCreate(NodeGraph* graph);
		void HandleDelete(NodeGraph* graph);
		void HandleContextMenus(NodeGraph* graph);
		void SyncSelection();
		void ApplyStoredPositions(NodeGraph* graph);
		void CapturePositions(NodeGraph* graph);

		uintptr_t AllocNodeId(const PluUUID& uuid);
		uintptr_t AllocPinId(const PluUUID& nodeUuid, const String& pinName, EPinDirection direction);
		uintptr_t AllocLinkId(const NodeLink& link);
		static String PinKey(const PluUUID& nodeUuid, const String& pinName, EPinDirection direction);
		static String LinkKey(const NodeLink& link);

		struct PinRef
		{
			PluUUID Node;
			String  Pin;
			EPinDirection Direction = EPinDirection::Input;
		};

		NodeViewRegistry mRegistry;

		uintptr_t mNextId = 1;
		GameHashMap<UInt64, uintptr_t>   mNodeIdByUuid;
		GameHashMap<uintptr_t, UInt64>   mNodeUuidById;
		GameHashMap<String, uintptr_t>   mPinIdByKey;
		GameHashMap<uintptr_t, PinRef>   mPinById;
		GameHashMap<String, uintptr_t>   mLinkIdByKey;
		GameHashMap<uintptr_t, NodeLink> mLinkById;

		GameHashMap<UInt64, ImVec2> mNodePositions;  // node uuid -> canvas position (layout)
		GameHashMap<UInt64, bool>   mPositionApplied; // nodes already pushed into ed this session

		ImVec2 mSpawnCanvasPos = ImVec2(0.0f, 0.0f); // where the next palette-added node lands
		ax::NodeEditor::NodeId mContextNode = 0;      // node under the context menu

		bool mHasSelection = false;
		PluUUID mSelectedNode;

		const std::function<void()>* mOnModified = nullptr; // valid only during Draw()
	};
}

#endif //PLUENGINE_NODEGRAPHEDITOR_H
