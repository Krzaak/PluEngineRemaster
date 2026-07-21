//
// Created by Plutex on 7/21/26.
//

#include "NodeGraph/NodeGraphEditor.h"

#include "PluEngine/NodeGraph/GraphNode.h"
#include "PluEngine/NodeGraph/NodeGraph.h"
#include "PluEngine/PluTypes.h"
#include "PluEngine/Reflection/ReflectionBase.h"
#include "PluEngine/Managers/DiskManager.h"
#include <cfloat>

namespace ed = ax::NodeEditor;

namespace Plu
{
	// ---- pin colors ---------------------------------------------------------------------------
	static ImVec4 PinColor(const NodePin& pin)
	{
		if (pin.Category == EPinCategory::Flow) return ImVec4(0.45f, 0.85f, 0.45f, 1.0f); // pose/exec wires
		return ImVec4(0.40f, 0.65f, 1.00f, 1.0f);                                          // data
	}

	static void DrawPinDot(const NodePin& pin)
	{
		ImGui::TextColored(PinColor(pin), "%s", "\xE2\x97\x8F"); // ● (U+25CF)
	}

	// ---- NodeViewRegistry ---------------------------------------------------------------------
	void NodeViewRegistry::Register(const String& typeName, INodeView* view)
	{
		if (view) mViews.Insert(typeName, view);
	}

	INodeView* NodeViewRegistry::For(GraphNode* node)
	{
		if (node) {
			if (INodeView** found = mViews.Find(node->GetClass()->TypeName)) return *found;
		}
		return &mDefaultView;
	}

	// ---- DefaultNodeView ----------------------------------------------------------------------
	void DefaultNodeView::Draw(GraphNode* node, NodeGraphEditor& editor)
	{
		ed::BeginNode(editor.NodeId(node));
		ImGui::PushID(node);
		DrawHeader(node, editor);
		DrawBody(node, editor);
		DrawPins(node, editor);
		ImGui::PopID();
		ed::EndNode();
	}

	void DefaultNodeView::DrawHeader(GraphNode* node, NodeGraphEditor& editor)
	{
		ImGui::TextUnformatted(node->GetDisplayName().CStr());
		ImGui::Dummy(ImVec2(0.0f, 2.0f));
	}

	void DefaultNodeView::DrawPins(GraphNode* node, NodeGraphEditor& editor)
	{
		for (NodePin& pin : node->InputPins) {
			ed::BeginPin(editor.PinId(node->Uuid, pin.Name, EPinDirection::Input), ed::PinKind::Input);
			DrawPinDot(pin);
			ImGui::SameLine();
			ImGui::TextUnformatted(pin.Name.CStr());
			ed::EndPin();
		}
		for (NodePin& pin : node->OutputPins) {
			ed::BeginPin(editor.PinId(node->Uuid, pin.Name, EPinDirection::Output), ed::PinKind::Output);
			ImGui::TextUnformatted(pin.Name.CStr());
			ImGui::SameLine();
			DrawPinDot(pin);
			ed::EndPin();
		}
	}

	// ---- NodeGraphEditor: ids -----------------------------------------------------------------
	String NodeGraphEditor::PinKey(const PluUUID& nodeUuid, const String& pinName, EPinDirection direction)
	{
		return String::FromInt(nodeUuid.getUUID()) + "|" + String::FromInt(static_cast<UInt64>(direction)) + "|" + pinName;
	}

	String NodeGraphEditor::LinkKey(const NodeLink& link)
	{
		return String::FromInt(link.FromNode.getUUID()) + "|" + link.FromPin
			+ ">" + String::FromInt(link.ToNode.getUUID()) + "|" + link.ToPin;
	}

	uintptr_t NodeGraphEditor::AllocNodeId(const PluUUID& uuid)
	{
		UInt64 key = uuid.getUUID();
		if (uintptr_t* found = mNodeIdByUuid.Find(key)) return *found;
		uintptr_t id = mNextId++;
		mNodeIdByUuid.Insert(key, id);
		mNodeUuidById.Insert(id, key);
		return id;
	}

	uintptr_t NodeGraphEditor::AllocPinId(const PluUUID& nodeUuid, const String& pinName, EPinDirection direction)
	{
		String key = PinKey(nodeUuid, pinName, direction);
		if (uintptr_t* found = mPinIdByKey.Find(key)) return *found;
		uintptr_t id = mNextId++;
		mPinIdByKey.Insert(key, id);
		mPinById.Insert(id, PinRef{ nodeUuid, pinName, direction });
		return id;
	}

	uintptr_t NodeGraphEditor::AllocLinkId(const NodeLink& link)
	{
		String key = LinkKey(link);
		if (uintptr_t* found = mLinkIdByKey.Find(key)) return *found;
		uintptr_t id = mNextId++;
		mLinkIdByKey.Insert(key, id);
		mLinkById.Insert(id, link);
		return id;
	}

	ax::NodeEditor::NodeId NodeGraphEditor::NodeId(const GraphNode* node)
	{
		return ed::NodeId(AllocNodeId(node->Uuid));
	}

	ax::NodeEditor::PinId NodeGraphEditor::PinId(const PluUUID& nodeUuid, const String& pinName, EPinDirection direction)
	{
		return ed::PinId(AllocPinId(nodeUuid, pinName, direction));
	}

	void NodeGraphEditor::MarkModified()
	{
		if (mOnModified && *mOnModified) (*mOnModified)();
	}

	// ---- NodeGraphEditor: frame ---------------------------------------------------------------
	void NodeGraphEditor::Draw(NodeGraph* graph, const std::function<void()>& onModified)
	{
		if (!graph) return;
		mOnModified = &onModified;

		ApplyStoredPositions(graph);
		DrawNodes(graph);
		DrawLinks(graph);
		HandleCreate(graph);
		HandleDelete(graph);
		HandleContextMenus(graph);
		SyncSelection();
		CapturePositions(graph);

		mOnModified = nullptr;
	}

	void NodeGraphEditor::DrawNodes(NodeGraph* graph)
	{
		for (TOwningPointer<GraphNode>& node : graph->Nodes) {
			if (!node) continue;
			INodeView* view = mRegistry.For(node.GetRaw());
			view->Draw(node.GetRaw(), *this);
		}
	}

	void NodeGraphEditor::DrawLinks(NodeGraph* graph)
	{
		for (const NodeLink& link : graph->Links) {
			uintptr_t linkId = AllocLinkId(link);
			ed::PinId out = ed::PinId(AllocPinId(link.FromNode, link.FromPin, EPinDirection::Output));
			ed::PinId in  = ed::PinId(AllocPinId(link.ToNode,   link.ToPin,   EPinDirection::Input));
			ed::Link(ed::LinkId(linkId), out, in);
		}
	}

	void NodeGraphEditor::HandleCreate(NodeGraph* graph)
	{
		if (ed::BeginCreate(ImVec4(0.5f, 0.7f, 1.0f, 1.0f), 2.0f)) {
			ed::PinId aId, bId;
			if (ed::QueryNewLink(&aId, &bId)) {
				if (aId && bId) {
					PinRef* ra = mPinById.Find(aId.Get());
					PinRef* rb = mPinById.Find(bId.Get());
					bool ok = false;
					PinRef outRef{}, inRef{};
					if (ra && rb && ra->Direction != rb->Direction) {
						PinRef* out = (ra->Direction == EPinDirection::Output) ? ra : rb;
						PinRef* in  = (ra->Direction == EPinDirection::Input)  ? ra : rb;
						GraphNode* fromNode = graph->FindNode(out->Node);
						GraphNode* toNode   = graph->FindNode(in->Node);
						if (fromNode && toNode && !(out->Node == in->Node)) {
							NodePin* op = fromNode->FindPin(out->Pin, EPinDirection::Output);
							NodePin* ip = toNode->FindPin(in->Pin, EPinDirection::Input);
							if (op && ip && NodePin::CanConnect(*op, *ip)) {
								ok = true;
								outRef = *out;
								inRef  = *in;
							}
						}
					}
					if (ok) {
						if (ed::AcceptNewItem(ImVec4(0.45f, 0.85f, 0.45f, 1.0f), 3.0f)) {
							if (graph->Connect(outRef.Node, outRef.Pin, inRef.Node, inRef.Pin)) MarkModified();
						}
					} else {
						ed::RejectNewItem(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), 2.0f);
					}
				}
			}
		}
		ed::EndCreate();
	}

	void NodeGraphEditor::HandleDelete(NodeGraph* graph)
	{
		if (ed::BeginDelete()) {
			ed::LinkId linkId;
			while (ed::QueryDeletedLink(&linkId)) {
				if (ed::AcceptDeletedItem()) {
					if (NodeLink* link = mLinkById.Find(linkId.Get())) {
						graph->Disconnect(*link);
						MarkModified();
					}
				}
			}
			ed::NodeId nodeId;
			while (ed::QueryDeletedNode(&nodeId)) {
				if (ed::AcceptDeletedItem()) {
					if (UInt64* uuid = mNodeUuidById.Find(nodeId.Get())) {
						graph->RemoveNode(PluUUID(*uuid));
						MarkModified();
					}
				}
			}
		}
		ed::EndDelete();
	}

	void NodeGraphEditor::HandleContextMenus(NodeGraph* graph)
	{
		ed::Suspend();
		if (ed::ShowNodeContextMenu(&mContextNode)) {
			ImGui::OpenPopup("node-ctx");
		} else if (ed::ShowBackgroundContextMenu()) {
			mSpawnCanvasPos = ed::ScreenToCanvas(ImGui::GetMousePos());
			ImGui::OpenPopup("add-node");
		}

		if (ImGui::BeginPopup("node-ctx")) {
			if (ImGui::MenuItem("Delete")) {
				if (UInt64* uuid = mNodeUuidById.Find(mContextNode.Get())) {
					graph->RemoveNode(PluUUID(*uuid));
					MarkModified();
				}
			}
			ImGui::EndPopup();
		}

		if (ImGui::BeginPopup("add-node")) {
			ImGui::TextDisabled("Add Node");
			ImGui::Separator();
			TypeInfo* base = graph->GetNodeBaseType();
			for (auto entry : *TypeRegistry::GetInstance()->GetTypeMap()) {
				TypeInfo* type = entry.second;
				if (!type || type->IsAbstract) continue;
				if (!type->IsDerivedOfOrSame(base)) continue;
				if (ImGui::MenuItem(type->TypeName.CStr())) {
					if (GraphNode* node = graph->AddNode(type)) {
						mNodePositions[node->Uuid.getUUID()] = mSpawnCanvasPos;
						MarkModified();
					}
				}
			}
			ImGui::EndPopup();
		}
		ed::Resume();
	}

	void NodeGraphEditor::SyncSelection()
	{
		ed::NodeId selected[1];
		int count = ed::GetSelectedNodes(selected, 1);
		if (count >= 1) {
			if (UInt64* uuid = mNodeUuidById.Find(selected[0].Get())) {
				mSelectedNode = PluUUID(*uuid);
				mHasSelection = true;
				return;
			}
		}
		mHasSelection = false;
	}

	void NodeGraphEditor::ApplyStoredPositions(NodeGraph* graph)
	{
		for (TOwningPointer<GraphNode>& node : graph->Nodes) {
			if (!node) continue;
			UInt64 key = node->Uuid.getUUID();
			if (mPositionApplied.Contains(key)) continue;

			uintptr_t id = AllocNodeId(node->Uuid);
			ImVec2 pos;
			ImVec2* stored = mNodePositions.Find(key);
			if (stored && stored->x < FLT_MAX && stored->y < FLT_MAX) {
				pos = *stored;
			} else {
				// No known layout for this node — cascade so fresh/unpositioned nodes don't stack.
				int n = static_cast<int>(mPositionApplied.Size());
				pos = ImVec2(40.0f + static_cast<float>(n % 6) * 190.0f,
				             40.0f + static_cast<float>(n / 6) * 130.0f);
				mNodePositions[key] = pos;
			}
			ed::SetNodePosition(ed::NodeId(id), pos);
			mPositionApplied.Insert(key, true);
		}
	}

	void NodeGraphEditor::CapturePositions(NodeGraph* graph)
	{
		for (TOwningPointer<GraphNode>& node : graph->Nodes) {
			if (!node) continue;
			UInt64 key = node->Uuid.getUUID();
			// Only capture nodes ed has actually laid out. A node added this frame (context menu runs
			// after DrawNodes) isn't drawn yet — ed::GetNodePosition returns (FLT_MAX, FLT_MAX) for it,
			// which would poison its stored position and fling it to infinity next frame (invisible +
			// breaks "F" navigate-to-content). Its spawn position is already stored; leave it be.
			if (!mPositionApplied.Contains(key)) continue;
			ImVec2 pos = ed::GetNodePosition(ed::NodeId(AllocNodeId(node->Uuid)));
			if (pos.x >= FLT_MAX || pos.y >= FLT_MAX) continue; // node not known to ed this frame
			mNodePositions[key] = pos;
		}
	}

	// ---- layout persistence (editor-owned sidecar) --------------------------------------------
	void NodeGraphEditor::LoadLayout(const StringW& path)
	{
		std::optional<JSON> opt = DiskManager::LoadJson(path);
		if (!opt.has_value()) return;
		const JSON& json = opt.value();
		if (!json.contains("positions")) return;
		for (const JSON& entry : json["positions"]) {
			UInt64 uuid = entry.value("uuid", static_cast<UInt64>(0));
			float x = entry.value("x", 0.0f);
			float y = entry.value("y", 0.0f);
			if (x >= FLT_MAX || y >= FLT_MAX) continue; // guard against previously-poisoned layouts
			mNodePositions[uuid] = ImVec2(x, y);
		}
	}

	void NodeGraphEditor::SaveLayout(const StringW& path) const
	{
		JSON json;
		json["positions"] = JSON::array();
		for (auto entry : mNodePositions) {
			JSON e;
			e["uuid"] = entry.first;
			e["x"] = entry.second.x;
			e["y"] = entry.second.y;
			json["positions"].push_back(e);
		}
		DiskManager::SaveJson(path, json);
	}
}
