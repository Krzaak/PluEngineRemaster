//
// Created by Plutex on 7/21/26.
//

#ifndef PLUENGINE_GRAPHNODE_H
#define PLUENGINE_GRAPHNODE_H

#include "PluEngine/Core.h"
#include "PluEngine/PluUUID.h"
#include "PluEngine/NodeGraph/NodePin.h"
#include "Array/Array.h"
#include "String/String.h"
#include "GraphNode.generated.h"

namespace Plu
{
	// Reusable base for every node in every graph domain. A plain reflected polymorphic class
	// (NOT an EngineObject) — same shape as IAssetData: we get factory-by-name (TypeInfo::Construct),
	// property enumeration for auto pin/param UI, and typeName-tagged polymorphic JSON, without the
	// per-object EngineObjectManager overhead. Domain category bases (e.g. AnimGraphNode) derive this
	// and add their flow-pin kind + evaluation contract; concrete nodes derive those.
	//
	// Editor-only concerns (canvas position, rendering) live in the editor, never here.
	//
	// A PLU_STRUCT (not PLU_CLASS): the generator gives PLU_CLASS an unconditional `GetClass()
	// override` assuming an EngineObject base, which a non-EngineObject root can't have. PLU_STRUCT
	// resolves virtual/override correctly for a reflected-struct hierarchy — same as IAssetData.
	PLU_STRUCT(Abstract)
	struct PLU_API GraphNode
	{
		REFLECTION_BODY_GRAPHNODE()
	public:
		GraphNode() = default;
		virtual ~GraphNode() = default;

		// Stable identity across saves; link endpoints reference it. Random by default (PluUUID ctor).
		PLU_PROPERTY()
		PluUUID Uuid;

		// (Re)built by BuildPins(), never serialized. Concrete nodes fill these.
		DynamicArray<NodePin> InputPins;
		DynamicArray<NodePin> OutputPins;

		// Declare this node's pins. Category bases add their flow pins; concrete nodes add the rest
		// (often via BuildDataPinsFromReflection()). Called on construction and on RebuildAllPins().
		virtual void BuildPins() {}

		// Title shown on the node header. Defaults to the reflected type name.
		virtual String GetDisplayName();

		// Adds one Data input pin per reflected PLU_PROPERTY whose type maps to a pin type
		// ("reszta pinów po typach z refleksji"). Skips the Uuid identity field.
		void BuildDataPinsFromReflection();

		NodePin* FindPin(const String& name, EPinDirection direction);

	protected:
		void AddPin(const String& name, EPinDirection direction, EPinCategory category, const String& typeId);
	};
}

#endif //PLUENGINE_GRAPHNODE_H
