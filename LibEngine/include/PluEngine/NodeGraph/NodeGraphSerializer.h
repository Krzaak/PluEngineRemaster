//
// Created by Plutex on 7/21/26.
//

#ifndef PLUENGINE_NODEGRAPHSERIALIZER_H
#define PLUENGINE_NODEGRAPHSERIALIZER_H

#include "PluEngine/Core.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
	struct NodeGraph;
	struct DeserializationContext;

	// Reusable, domain-agnostic save/load for a NodeGraph's polymorphic node list and its links.
	// Kept out of reflection because the concrete node type of each element must be preserved
	// (each node is written with its typeName via TypeSerializer<TypeInfo*>). Any concrete graph
	// asset loader (AnimationGraphAssetLoader, future logic-graph loaders, ...) delegates here.
	class PLU_API NodeGraphSerializer
	{
	public:
		// Writes json["nodes"] (each: typeName + fields incl. Uuid) and json["links"].
		static void Save(NodeGraph& graph, JSON& outJson);

		// Reconstructs nodes by typeName and repopulates links. Clears the graph first.
		static void Load(DeserializationContext* dc, NodeGraph& graph, const JSON& json);
	};
}

#endif //PLUENGINE_NODEGRAPHSERIALIZER_H
