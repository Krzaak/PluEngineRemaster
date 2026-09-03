//
// Created by Plutex on 7/25/26.
//

#ifndef PLUENGINE_DATAGRAPHNODE_H
#define PLUENGINE_DATAGRAPHNODE_H

#include "PluEngine/Core.h"
#include "PluEngine/AssetTypes/NodeGraph/GraphNode.h"
#include "PluEngine/AssetTypes/NodeGraph/GraphValue.h"
#include "DataGraphNode.generated.h"

namespace Plu
{
	// Category base for pure-value nodes: no flow pins, no domain state — they only read data pins and
	// produce data outputs. Because they touch nothing animation-specific, NodeGraph::AcceptsNodeType
	// lets them into *every* graph domain, present and future (see NodeGraph::AcceptsNodeType).
	//
	// Wildcard typing: instead of one node type per (operation, value type), a node carries ValueType
	// and rebuilds its pins for it. Pin retyping happens in BuildPins, which the editor re-runs after
	// any property edit (AnimationGraphDetailsPanel) — links that stop type-checking are pruned there.
	PLU_STRUCT(Abstract)
	struct PLUASSETTYPES_API DataGraphNode : GraphNode
	{
		REFLECTION_BODY_DATAGRAPHNODE()

		// Pin names shared by the value nodes. Constants, not literals, because pin names are link
		// identity — renaming one silently breaks every saved graph that used it.
		static constexpr const char* ResultPin = "Result";

		// The value type this node currently operates on. Clamped to SupportedTypes() by BuildPins,
		// so picking an unsupported entry in the details-panel combo can't leave the node broken.
		PLU_PROPERTY()
		EGraphValueType ValueType = EGraphValueType::Float;

		// Bitmask (1 << EGraphValueType) of the types this node can operate on. Default: the numeric
		// ones — bool arithmetic is meaningless, so a node has to opt in (Constant, Select do).
		[[nodiscard]] virtual UInt32 SupportedTypes() const
		{
			return TypeBit(EGraphValueType::Float) | TypeBit(EGraphValueType::Int) | TypeBit(EGraphValueType::Vec3);
		}

		// Node title: the operation plus the active type, e.g. "Add (Vec3)" — without it every
		// wildcard node on the canvas looks identical.
		String GetDisplayName() override
		{
			String name = OperationName();
			if (SupportsMultipleTypes()) {
				name += " (";
				name += ValueTypeName(ValueType);
				name += ")";
			}
			return name;
		}

		// Human-readable operation label ("Add", "Dot", ...). Defaults to the reflected type name.
		[[nodiscard]] virtual String OperationName() { return GraphNode::GetDisplayName(); }

		static constexpr UInt32 TypeBit(EGraphValueType type) { return 1u << static_cast<UInt32>(type); }

	protected:
		[[nodiscard]] bool Supports(EGraphValueType type) const { return (SupportedTypes() & TypeBit(type)) != 0; }

		// Called first thing in BuildPins: snaps ValueType onto a supported type and mirrors it into
		// the node's inline defaults so the details panel edits the value the pins actually carry.
		void ClampValueType()
		{
			if (Supports(ValueType)) return;
			for (UInt32 i = 0; i <= static_cast<UInt32>(EGraphValueType::Vec3); ++i) {
				const auto candidate = static_cast<EGraphValueType>(i);
				if (Supports(candidate)) { ValueType = candidate; return; }
			}
		}

		void AddValueInput(const String& name, EGraphValueType type)
		{
			AddPin(name, EPinDirection::Input, EPinCategory::Data, GraphValue::PinTypeId(type));
		}

		void AddValueOutput(const String& name, EGraphValueType type)
		{
			AddPin(name, EPinDirection::Output, EPinCategory::Data, GraphValue::PinTypeId(type));
		}

		// Reads both operand pins in the node's active type, applies Op's matching overload and writes
		// the result into `outValue`. The unconnected-pin fallback is the node's own authored default
		// (aDefault/bDefault), exactly like AnimBlendNode::Alpha.
		template <typename Op>
		bool EvalBinary(GraphEvalContext& context, void* outValue, EGraphValueType type,
		                const String& aPin, const GraphValue& aDefault,
		                const String& bPin, const GraphValue& bDefault) const
		{
			const Op op{};
			switch (type) {
				case EGraphValueType::Float:
					*static_cast<float*>(outValue) = op(ReadDataPin<float>(context, aPin, aDefault.Float),
					                                    ReadDataPin<float>(context, bPin, bDefault.Float));
					return true;
				case EGraphValueType::Int:
					*static_cast<int*>(outValue) = op(ReadDataPin<int>(context, aPin, aDefault.Int),
					                                 ReadDataPin<int>(context, bPin, bDefault.Int));
					return true;
				case EGraphValueType::Vec3:
					*static_cast<Vec3*>(outValue) = op(ReadDataPin<Vec3>(context, aPin, aDefault.Vector),
					                                   ReadDataPin<Vec3>(context, bPin, bDefault.Vector));
					return true;
				case EGraphValueType::Bool:
					return false; // arithmetic on bools is meaningless — see SupportedTypes()
			}
			return false;
		}

		template <typename Op>
		bool EvalUnary(GraphEvalContext& context, void* outValue, EGraphValueType type,
		               const String& valuePin, const GraphValue& valueDefault) const
		{
			const Op op{};
			switch (type) {
				case EGraphValueType::Float:
					*static_cast<float*>(outValue) = op(ReadDataPin<float>(context, valuePin, valueDefault.Float));
					return true;
				case EGraphValueType::Int:
					*static_cast<int*>(outValue) = op(ReadDataPin<int>(context, valuePin, valueDefault.Int));
					return true;
				case EGraphValueType::Vec3:
					*static_cast<Vec3*>(outValue) = op(ReadDataPin<Vec3>(context, valuePin, valueDefault.Vector));
					return true;
				case EGraphValueType::Bool:
					return false;
			}
			return false;
		}

		// Guard every EvaluateDataOutput override starts with: the asked-for pin must be this node's
		// output and the requested type must be the one its pins currently carry.
		[[nodiscard]] bool IsOutput(const String& pinName, const String& typeId,
		                            const String& expectedPin, EGraphValueType expectedType) const
		{
			return pinName == expectedPin && typeId == GraphValue::PinTypeId(expectedType);
		}

		[[nodiscard]] bool SupportsMultipleTypes() const
		{
			const UInt32 mask = SupportedTypes();
			return (mask & (mask - 1)) != 0;
		}

		static const char* ValueTypeName(EGraphValueType type)
		{
			switch (type) {
				case EGraphValueType::Float: return "Float";
				case EGraphValueType::Int:   return "Int";
				case EGraphValueType::Bool:  return "Bool";
				case EGraphValueType::Vec3:  return "Vec3";
			}
			return "Float";
		}
	};

	// The four palette categories. They carry no behaviour — the editor derives the "Add Node" menu
	// grouping from the nearest abstract ancestor, so a node's category is simply which of these it
	// derives from (see NodeGraphEditor's add-node popup).
	PLU_STRUCT(Abstract)
	struct PLUASSETTYPES_API MathGraphNode : DataGraphNode
	{
		REFLECTION_BODY_MATHGRAPHNODE()
	};

	PLU_STRUCT(Abstract)
	struct PLUASSETTYPES_API VectorGraphNode : DataGraphNode
	{
		REFLECTION_BODY_VECTORGRAPHNODE()

		// Vector nodes have fixed pin types; the wildcard combo would be noise.
		[[nodiscard]] UInt32 SupportedTypes() const override { return TypeBit(EGraphValueType::Vec3); }
	};

	PLU_STRUCT(Abstract)
	struct PLUASSETTYPES_API LogicGraphNode : DataGraphNode
	{
		REFLECTION_BODY_LOGICGRAPHNODE()

		[[nodiscard]] UInt32 SupportedTypes() const override { return TypeBit(EGraphValueType::Bool); }
	};

	PLU_STRUCT(Abstract)
	struct PLUASSETTYPES_API ConvertGraphNode : DataGraphNode
	{
		REFLECTION_BODY_CONVERTGRAPHNODE()

		// Conversions are the one place where the pin types on the two sides differ on purpose, so
		// they pin both ends themselves and never show the type combo.
		[[nodiscard]] UInt32 SupportedTypes() const override { return TypeBit(EGraphValueType::Float); }
	};
}

#endif //PLUENGINE_DATAGRAPHNODE_H
