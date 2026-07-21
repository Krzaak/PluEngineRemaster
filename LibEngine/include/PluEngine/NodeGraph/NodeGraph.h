//
// Created by Plutex on 7/21/26.
//

#ifndef PLUENGINE_NODEGRAPH_H
#define PLUENGINE_NODEGRAPH_H

#include "PluEngine/Core.h"
#include "PluEngine/Managers/AssetsManager.h"
#include "PluEngine/NodeGraph/GraphNode.h"
#include "PluEngine/NodeGraph/NodeLink.h"
#include "Array/Array.h"
#include "Pointers/TOwningPointer.h"
#include "NodeGraph.generated.h"

namespace Plu
{
	struct TypeInfo;

	// Reusable node-graph asset base: owns a polymorphic node list + the links between them.
	// Concrete graph assets (e.g. AnimationGraph) derive this and override GetNodeBaseType() so the
	// editor palette lists the right node family. Serialization of Nodes/Links is done by
	// NodeGraphSerializer (the generic reflection array serializer is non-polymorphic and would drop
	// subclass identity) — hence Nodes/Links are deliberately NOT PLU_PROPERTY.
	PLU_STRUCT()
	struct PLU_API NodeGraph : IAssetData
	{
		REFLECTION_BODY_NODEGRAPH()

		DynamicArray<TOwningPointer<GraphNode>> Nodes;
		DynamicArray<NodeLink> Links;

		~NodeGraph() override = default;

		// Node family this graph accepts. Drives the "Add Node" palette. Default: any GraphNode.
		virtual TypeInfo* GetNodeBaseType();

		// Constructs a node of nodeType (must derive GetNodeBaseType()), builds its pins, appends it.
		// Returns a non-owning view (owned by Nodes). Null if nodeType is null / not constructible.
		GraphNode* AddNode(TypeInfo* nodeType);

		// Removes the node and every link touching it.
		void RemoveNode(const PluUUID& nodeId);

		GraphNode* FindNode(const PluUUID& nodeId);

		// Adds a link if the two pins are compatible (NodePin::CanConnect) and not already linked.
		// Pins are addressed by (node uuid, pin name). Returns whether a link was added.
		bool Connect(const PluUUID& fromNode, const String& fromPin,
		             const PluUUID& toNode, const String& toPin);

		void Disconnect(const NodeLink& link);

		// Clears and rebuilds every node's pins (after edits that change pin topology).
		void RebuildAllPins();
	};
}

#endif //PLUENGINE_NODEGRAPH_H
