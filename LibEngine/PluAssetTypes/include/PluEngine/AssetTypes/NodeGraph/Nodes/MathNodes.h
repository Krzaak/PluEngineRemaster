//
// Created by Plutex on 7/25/26.
//

#ifndef PLUENGINE_MATHNODES_H
#define PLUENGINE_MATHNODES_H

#include "PluEngine/Core.h"
#include "PluEngine/AssetTypes/NodeGraph/Nodes/DataGraphNode.h"
#include "PluEngine/AssetTypes/NodeGraph/Nodes/GraphNodeMathOps.h"
#include "MathNodes.generated.h"

namespace Plu
{
	// Two operands of the node's active type in, one of the same type out. Concrete operations only
	// have to name themselves and pick a functor — see MathAddNode right below.
	PLU_STRUCT(Abstract)
	struct PLUASSETTYPES_API MathBinaryNode : MathGraphNode
	{
		REFLECTION_BODY_MATHBINARYNODE()

		static constexpr const char* APin = "A";
		static constexpr const char* BPin = "B";

		// Inline defaults for the pins while they are unconnected; the details panel edits these.
		PLU_PROPERTY()
		GraphValue A;

		PLU_PROPERTY()
		GraphValue B;

		void BuildPins() override
		{
			ClampValueType();
			A.Type = ValueType;
			B.Type = ValueType;
			AddValueInput(APin, ValueType);
			AddValueInput(BPin, ValueType);
			AddValueOutput(ResultPin, ValueType);
		}

	protected:
		template <typename Op>
		bool EvalResult(GraphEvalContext& context, const String& pinName, const String& typeId, void* outValue)
		{
			if (!IsOutput(pinName, typeId, ResultPin, ValueType)) return false;
			return EvalBinary<Op>(context, outValue, ValueType, APin, A, BPin, B);
		}
	};

	// One operand in, one of the same type out.
	PLU_STRUCT(Abstract)
	struct PLUASSETTYPES_API MathUnaryNode : MathGraphNode
	{
		REFLECTION_BODY_MATHUNARYNODE()

		static constexpr const char* ValuePin = "Value";

		PLU_PROPERTY()
		GraphValue Value;

		void BuildPins() override
		{
			ClampValueType();
			Value.Type = ValueType;
			AddValueInput(ValuePin, ValueType);
			AddValueOutput(ResultPin, ValueType);
		}

	protected:
		template <typename Op>
		bool EvalResult(GraphEvalContext& context, const String& pinName, const String& typeId, void* outValue)
		{
			if (!IsOutput(pinName, typeId, ResultPin, ValueType)) return false;
			return EvalUnary<Op>(context, outValue, ValueType, ValuePin, Value);
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API MathAddNode : MathBinaryNode
	{
		REFLECTION_BODY_MATHADDNODE()

		String OperationName() override { return "Add"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			return EvalResult<GraphOps::Add>(context, pinName, typeId, outValue);
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API MathSubtractNode : MathBinaryNode
	{
		REFLECTION_BODY_MATHSUBTRACTNODE()

		String OperationName() override { return "Subtract"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			return EvalResult<GraphOps::Subtract>(context, pinName, typeId, outValue);
		}
	};

	// Vec3 × Vec3 is component-wise; scaling a vector by a scalar is VectorScaleNode.
	PLU_STRUCT()
	struct PLUASSETTYPES_API MathMultiplyNode : MathBinaryNode
	{
		REFLECTION_BODY_MATHMULTIPLYNODE()

		String OperationName() override { return "Multiply"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			return EvalResult<GraphOps::Multiply>(context, pinName, typeId, outValue);
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API MathDivideNode : MathBinaryNode
	{
		REFLECTION_BODY_MATHDIVIDENODE()

		String OperationName() override { return "Divide"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			return EvalResult<GraphOps::Divide>(context, pinName, typeId, outValue);
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API MathMinNode : MathBinaryNode
	{
		REFLECTION_BODY_MATHMINNODE()

		String OperationName() override { return "Min"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			return EvalResult<GraphOps::Min>(context, pinName, typeId, outValue);
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API MathMaxNode : MathBinaryNode
	{
		REFLECTION_BODY_MATHMAXNODE()

		String OperationName() override { return "Max"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			return EvalResult<GraphOps::Max>(context, pinName, typeId, outValue);
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API MathAbsNode : MathUnaryNode
	{
		REFLECTION_BODY_MATHABSNODE()

		String OperationName() override { return "Abs"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			return EvalResult<GraphOps::Abs>(context, pinName, typeId, outValue);
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API MathNegateNode : MathUnaryNode
	{
		REFLECTION_BODY_MATHNEGATENODE()

		String OperationName() override { return "Negate"; }

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			return EvalResult<GraphOps::Negate>(context, pinName, typeId, outValue);
		}
	};

	// Value clamped into [Min, Max], component-wise for Vec3.
	PLU_STRUCT()
	struct PLUASSETTYPES_API MathClampNode : MathGraphNode
	{
		REFLECTION_BODY_MATHCLAMPNODE()

		static constexpr const char* ValuePin = "Value";
		static constexpr const char* MinPin   = "Min";
		static constexpr const char* MaxPin   = "Max";

		PLU_PROPERTY()
		GraphValue Value;

		PLU_PROPERTY()
		GraphValue Min;

		PLU_PROPERTY()
		GraphValue Max;

		String OperationName() override { return "Clamp"; }

		void BuildPins() override
		{
			ClampValueType();
			Value.Type = ValueType;
			Min.Type   = ValueType;
			Max.Type   = ValueType;
			AddValueInput(ValuePin, ValueType);
			AddValueInput(MinPin, ValueType);
			AddValueInput(MaxPin, ValueType);
			AddValueOutput(ResultPin, ValueType);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, ValueType)) return false;
			switch (ValueType) {
				case EGraphValueType::Float:
					*static_cast<float*>(outValue) = glm::clamp(ReadDataPin<float>(context, ValuePin, Value.Float),
					                                            ReadDataPin<float>(context, MinPin, Min.Float),
					                                            ReadDataPin<float>(context, MaxPin, Max.Float));
					return true;
				case EGraphValueType::Int:
					*static_cast<int*>(outValue) = glm::clamp(ReadDataPin<int>(context, ValuePin, Value.Int),
					                                          ReadDataPin<int>(context, MinPin, Min.Int),
					                                          ReadDataPin<int>(context, MaxPin, Max.Int));
					return true;
				case EGraphValueType::Vec3:
					*static_cast<Vec3*>(outValue) = glm::clamp(ReadDataPin<Vec3>(context, ValuePin, Value.Vector),
					                                           ReadDataPin<Vec3>(context, MinPin, Min.Vector),
					                                           ReadDataPin<Vec3>(context, MaxPin, Max.Vector));
					return true;
				case EGraphValueType::Bool:
					return false;
			}
			return false;
		}
	};

	// Linear blend between A and B. Alpha is always a float pin regardless of the operand type — it is
	// the same "blend weight" concept as AnimBlendNode::Alpha, and unclamped on purpose (extrapolation
	// is occasionally useful; clamp with a Clamp node when it isn't).
	PLU_STRUCT()
	struct PLUASSETTYPES_API MathLerpNode : MathGraphNode
	{
		REFLECTION_BODY_MATHLERPNODE()

		static constexpr const char* APin     = "A";
		static constexpr const char* BPin     = "B";
		static constexpr const char* AlphaPin = "Alpha";

		PLU_PROPERTY()
		GraphValue A;

		PLU_PROPERTY()
		GraphValue B;

		PLU_PROPERTY()
		float Alpha = 0.0f;

		String OperationName() override { return "Lerp"; }

		// Int lerping rounds toward zero at the cast, which is rarely what anyone means.
		[[nodiscard]] UInt32 SupportedTypes() const override
		{
			return TypeBit(EGraphValueType::Float) | TypeBit(EGraphValueType::Vec3);
		}

		void BuildPins() override
		{
			ClampValueType();
			A.Type = ValueType;
			B.Type = ValueType;
			AddValueInput(APin, ValueType);
			AddValueInput(BPin, ValueType);
			AddValueInput(AlphaPin, EGraphValueType::Float);
			AddValueOutput(ResultPin, ValueType);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, ValueType)) return false;
			const float alpha = ReadDataPin<float>(context, AlphaPin, Alpha);
			switch (ValueType) {
				case EGraphValueType::Float:
					*static_cast<float*>(outValue) = glm::mix(ReadDataPin<float>(context, APin, A.Float),
					                                          ReadDataPin<float>(context, BPin, B.Float), alpha);
					return true;
				case EGraphValueType::Vec3:
					*static_cast<Vec3*>(outValue) = glm::mix(ReadDataPin<Vec3>(context, APin, A.Vector),
					                                         ReadDataPin<Vec3>(context, BPin, B.Vector), alpha);
					return true;
				default:
					return false;
			}
		}
	};

	// A literal of any value type. The one node whose whole job is its inline default.
	PLU_STRUCT()
	struct PLUASSETTYPES_API MathConstantNode : MathGraphNode
	{
		REFLECTION_BODY_MATHCONSTANTNODE()

		PLU_PROPERTY()
		GraphValue Value;

		String OperationName() override { return "Constant"; }

		[[nodiscard]] UInt32 SupportedTypes() const override
		{
			return TypeBit(EGraphValueType::Float) | TypeBit(EGraphValueType::Int)
			     | TypeBit(EGraphValueType::Bool)  | TypeBit(EGraphValueType::Vec3);
		}

		void BuildPins() override
		{
			ClampValueType();
			Value.Type = ValueType;
			AddValueOutput(ResultPin, ValueType);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (pinName != ResultPin) return false;
			return Value.CopyTo(outValue, typeId);
		}
	};
}

#endif //PLUENGINE_MATHNODES_H
