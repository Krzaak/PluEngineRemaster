//
// Created by Plutex on 7/21/26.
//

#include "PluEngine/AssetTypes/NodeGraph/NodeGraphSerializer.h"

#include "PluEngine/AssetTypes/NodeGraph/NodeGraph.h"
#include "PluEngine/Core/Reflection/ReflectionBase.h"
#include "PluEngine/Core/Reflection/TypeTraits.h"

namespace Plu
{
	void NodeGraphSerializer::Save(NodeGraph& graph, JSON& outJson)
	{
		outJson["nodes"] = JSON::array();
		for (TOwningPointer<GraphNode>& node : graph.Nodes) {
			if (!node) continue;
			// Polymorphic: each node carries its concrete typeName + reflected fields (incl. Uuid).
			outJson["nodes"].push_back(TypeSerializer<TypeInfo*>::Serialize(node->GetClass(), node.GetRaw()));
		}

		outJson["links"] = JSON::array();
		for (const NodeLink& link : graph.Links) {
			JSON j;
			j["fromNode"] = link.FromNode.getUUID();
			j["fromPin"]  = link.FromPin.CStr();
			j["toNode"]   = link.ToNode.getUUID();
			j["toPin"]    = link.ToPin.CStr();
			outJson["links"].push_back(j);
		}
	}

	void NodeGraphSerializer::Load(DeserializationContext* dc, NodeGraph& graph, const JSON& json)
	{
		graph.Nodes.Clear();
		graph.Links.Clear();

		if (json.contains("nodes")) {
			for (const JSON& nodeJson : json["nodes"]) {
				if (!nodeJson.contains("typeName")) continue;
				String typeName = nodeJson["typeName"].get<std::string>().c_str();
				TypeInfo* type = TypeRegistry::GetInstance()->GetTypeOfName(typeName);
				if (!type) {
					PLU_CORE_WARN("NodeGraph load: unknown node type '{}', skipping", typeName.CStr());
					continue;
				}
				GraphNode* node = static_cast<GraphNode*>(type->Construct());
				if (!node) continue;
				// Fields (incl. Uuid) before pins, so pin topology that depends on params is correct.
				TypeSerializer<TypeInfo*>::Deserialize(dc, nodeJson, type, node);
				node->BuildPins();
				graph.Nodes.PushBack(TOwningPointer<GraphNode>(node));
			}
		}

		if (json.contains("links")) {
			for (const JSON& linkJson : json["links"]) {
				NodeLink link;
				link.FromNode = PluUUID(linkJson.value("fromNode", static_cast<UInt64>(0)));
				link.FromPin  = linkJson.value("fromPin", std::string()).c_str();
				link.ToNode   = PluUUID(linkJson.value("toNode", static_cast<UInt64>(0)));
				link.ToPin    = linkJson.value("toPin", std::string()).c_str();
				graph.Links.PushBack(link);
			}
		}
	}
}
