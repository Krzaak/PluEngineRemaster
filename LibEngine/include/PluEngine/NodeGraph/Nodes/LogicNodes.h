//
// Created by Plutex on 7/25/26.
//

#ifndef PLUENGINE_LOGICNODES_H
#define PLUENGINE_LOGICNODES_H

#include "PluEngine/Core.h"
#include "PluEngine/NodeGraph/Nodes/DataGraphNode.h"
#include "PluEngine/NodeGraph/Nodes/GraphNodeMathOps.h"

#include "LogicNodes.generated.h"

namespace Plu
{
	// Two bool operands in, one bool out.
	PLU_STRUCT(Abstract)
	struct PLU_API LogicBinaryNode : LogicGraphNode
	{
		REFLECTION_BODY_LOGICBINARYNODE()

		static constexpr const char* APin = "A";
		static constexpr const char* BPin = "B";

		PLU_PROPERTY()
		bool A = false;

		PLU_PROPERTY()
		bool B = false;

		void BuildPins() override
		{
			AddValueInput(APin, EGraphValueType::Bool);
			AddValueInput(BPin, EGraphValueType::Bool);
			AddValueOutput(ResultPin, EGraphValueType::Bool);
		}
	};

	PLU_STRUCT()
	struct PLU_API LogicAndNode : LogicBinaryNode
	{
		REFLECTION_BODY_LOGICANDNODE()

		String OperationName() override { return "And"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Bool)) return false;
			*static_cast<bool*>(outValue) = ReadDataPin<bool>(context, APin, A)
			                             && ReadDataPin<bool>(context, BPin, B);
			return true;
		}
	};

	PLU_STRUCT()
	struct PLU_API LogicOrNode : LogicBinaryNode
	{
		REFLECTION_BODY_LOGICORNODE()

		String OperationName() override { return "Or"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Bool)) return false;
			*static_cast<bool*>(outValue) = ReadDataPin<bool>(context, APin, A)
			                             || ReadDataPin<bool>(context, BPin, B);
			return true;
		}
	};

	PLU_STRUCT()
	struct PLU_API LogicNotNode : LogicGraphNode
	{
		REFLECTION_BODY_LOGICNOTNODE()

		static constexpr const char* ValuePin = "Value";

		PLU_PROPERTY()
		bool Value = false;

		String OperationName() override { return "Not"; }

		void BuildPins() override
		{
			AddValueInput(ValuePin, EGraphValueType::Bool);
			AddValueOutput(ResultPin, EGraphValueType::Bool);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Bool)) return false;
			*static_cast<bool*>(outValue) = !ReadDataPin<bool>(context, ValuePin, Value);
			return true;
		}
	};

	// Comparisons: the operands are wildcard (float or int), the result is always bool — the one place
	// where the node's ValueType describes its inputs rather than its output.
	PLU_STRUCT(Abstract)
	struct PLU_API LogicCompareNode : LogicGraphNode
	{
		REFLECTION_BODY_LOGICCOMPARENODE()

		static constexpr const char* APin = "A";
		static constexpr const char* BPin = "B";

		PLU_PROPERTY()
		GraphValue A;

		PLU_PROPERTY()
		GraphValue B;

		[[nodiscard]] UInt32 SupportedTypes() const override
		{
			return TypeBit(EGraphValueType::Float) | TypeBit(EGraphValueType::Int);
		}

		void BuildPins() override
		{
			ClampValueType();
			A.Type = ValueType;
			B.Type = ValueType;
			AddValueInput(APin, ValueType);
			AddValueInput(BPin, ValueType);
			AddValueOutput(ResultPin, EGraphValueType::Bool);
		}

	protected:
		// Reads both operands in the active type and hands them to Compare as floats — int operands
		// convert exactly (they come from int pins, so no precision is lost at these magnitudes).
		template <typename Compare>
		bool EvalCompare(GraphEvalContext& context, const String& pinName, const String& typeId, void* outValue)
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Bool)) return false;
			const Compare compare{};
			switch (ValueType) {
				case EGraphValueType::Int:
					*static_cast<bool*>(outValue) = compare(ReadDataPin<int>(context, APin, A.Int),
					                                        ReadDataPin<int>(context, BPin, B.Int));
					return true;
				default:
					*static_cast<bool*>(outValue) = compare(ReadDataPin<float>(context, APin, A.Float),
					                                        ReadDataPin<float>(context, BPin, B.Float));
					return true;
			}
		}
	};

	PLU_STRUCT()
	struct PLU_API LogicGreaterNode : LogicCompareNode
	{
		REFLECTION_BODY_LOGICGREATERNODE()

		String OperationName() override { return "Greater"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			return EvalCompare<GraphOps::Greater>(context, pinName, typeId, outValue);
		}
	};

	PLU_STRUCT()
	struct PLU_API LogicLessNode : LogicCompareNode
	{
		REFLECTION_BODY_LOGICLESSNODE()

		String OperationName() override { return "Less"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			return EvalCompare<GraphOps::Less>(context, pinName, typeId, outValue);
		}
	};

	PLU_STRUCT()
	struct PLU_API LogicEqualNode : LogicCompareNode
	{
		REFLECTION_BODY_LOGICEQUALNODE()

		String OperationName() override { return "Equal"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			return EvalCompare<GraphOps::Equal>(context, pinName, typeId, outValue);
		}
	};

	// Condition ? A : B, with A/B of any value type. Both branches are evaluated (the graph is pull
	// based, there is no short-circuiting) — keep expensive work out of Select's inputs.
	PLU_STRUCT()
	struct PLU_API LogicSelectNode : LogicGraphNode
	{
		REFLECTION_BODY_LOGICSELECTNODE()

		static constexpr const char* ConditionPin = "Condition";
		static constexpr const char* APin = "A";
		static constexpr const char* BPin = "B";

		PLU_PROPERTY()
		bool Condition = false;

		PLU_PROPERTY()
		GraphValue A;

		PLU_PROPERTY()
		GraphValue B;

		String OperationName() override { return "Select"; }

		[[nodiscard]] UInt32 SupportedTypes() const override
		{
			return TypeBit(EGraphValueType::Float) | TypeBit(EGraphValueType::Int)
			     | TypeBit(EGraphValueType::Bool)  | TypeBit(EGraphValueType::Vec3);
		}

		void BuildPins() override
		{
			ClampValueType();
			A.Type = ValueType;
			B.Type = ValueType;
			AddValueInput(ConditionPin, EGraphValueType::Bool);
			AddValueInput(APin, ValueType);
			AddValueInput(BPin, ValueType);
			AddValueOutput(ResultPin, ValueType);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, ValueType)) return false;

			const bool condition = ReadDataPin<bool>(context, ConditionPin, Condition);
			const String pickedPin = condition ? APin : BPin;
			const GraphValue& pickedDefault = condition ? A : B;
			switch (ValueType) {
				case EGraphValueType::Float:
					*static_cast<float*>(outValue) = ReadDataPin<float>(context, pickedPin, pickedDefault.Float);
					return true;
				case EGraphValueType::Int:
					*static_cast<int*>(outValue) = ReadDataPin<int>(context, pickedPin, pickedDefault.Int);
					return true;
				case EGraphValueType::Bool:
					*static_cast<bool*>(outValue) = ReadDataPin<bool>(context, pickedPin, pickedDefault.Bool);
					return true;
				case EGraphValueType::Vec3:
					*static_cast<Vec3*>(outValue) = ReadDataPin<Vec3>(context, pickedPin, pickedDefault.Vector);
					return true;
			}
			return false;
		}
	};
}

#endif //PLUENGINE_LOGICNODES_H
