//
// Created by Plutex on 7/21/26.
//

#include "PluEngine/NodeGraph/NodeGraph.h"
#include "PluEngine/Reflection/ReflectionBase.h"

namespace Plu
{
	TypeInfo* NodeGraph::GetNodeBaseType()
	{
		return GraphNode::GetStaticClass();
	}

	GraphNode* NodeGraph::AddNode(TypeInfo* nodeType)
	{
		if (!nodeType) return nullptr;
		if (!nodeType->IsDerivedOfOrSame(GetNodeBaseType())) {
			PLU_CORE_WARN("AddNode: {} is not a {}", nodeType->TypeName.CStr(), GetNodeBaseType()->TypeName.CStr());
			return nullptr;
		}
		GraphNode* node = static_cast<GraphNode*>(nodeType->Construct());
		if (!node) return nullptr;
		node->BuildPins();
		Nodes.PushBack(TOwningPointer<GraphNode>(node));
		return node;
	}

	GraphNode* NodeGraph::FindNode(const PluUUID& nodeId)
	{
		for (TOwningPointer<GraphNode>& node : Nodes) {
			if (node && node->Uuid == nodeId) return node.GetRaw();
		}
		return nullptr;
	}

	void NodeGraph::RemoveNode(const PluUUID& nodeId)
	{
		// Drop every link touching the node first (iterate backward — RemoveAt shifts the tail).
		for (Int64 i = static_cast<Int64>(Links.Size()) - 1; i >= 0; --i) {
			const NodeLink& link = Links[i];
			if (link.FromNode == nodeId || link.ToNode == nodeId) {
				Links.RemoveAt(i);
			}
		}
		for (Int64 i = static_cast<Int64>(Nodes.Size()) - 1; i >= 0; --i) {
			if (Nodes[i] && Nodes[i]->Uuid == nodeId) {
				Nodes.RemoveAt(i);
			}
		}
	}

	bool NodeGraph::Connect(const PluUUID& fromNode, const String& fromPin,
	                        const PluUUID& toNode, const String& toPin)
	{
		if (fromNode == toNode) return false; // no self-links

		GraphNode* from = FindNode(fromNode);
		GraphNode* to   = FindNode(toNode);
		if (!from || !to) return false;

		NodePin* outPin = from->FindPin(fromPin, EPinDirection::Output);
		NodePin* inPin  = to->FindPin(toPin, EPinDirection::Input);
		if (!outPin || !inPin) return false;
		if (!NodePin::CanConnect(*outPin, *inPin)) return false;

		// An input accepts a single source: drop any existing link into it first.
		for (Int64 i = static_cast<Int64>(Links.Size()) - 1; i >= 0; --i) {
			if (Links[i].ToNode == toNode && Links[i].ToPin == toPin) {
				Links.RemoveAt(i);
			}
		}

		NodeLink link;
		link.FromNode = fromNode;
		link.FromPin  = fromPin;
		link.ToNode   = toNode;
		link.ToPin    = toPin;
		Links.PushBack(link);
		return true;
	}

	void NodeGraph::Disconnect(const NodeLink& link)
	{
		for (Int64 i = static_cast<Int64>(Links.Size()) - 1; i >= 0; --i) {
			if (Links[i] == link) {
				Links.RemoveAt(i);
			}
		}
	}

	void NodeGraph::RebuildAllPins()
	{
		for (TOwningPointer<GraphNode>& node : Nodes) {
			if (!node) continue;
			node->InputPins.Clear();
			node->OutputPins.Clear();
			node->BuildPins();
		}
	}
}
