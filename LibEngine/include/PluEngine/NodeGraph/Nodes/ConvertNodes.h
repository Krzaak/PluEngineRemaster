//
// Created by Plutex on 7/25/26.
//

#ifndef PLUENGINE_CONVERTNODES_H
#define PLUENGINE_CONVERTNODES_H

#include "PluEngine/Core.h"
#include "PluEngine/NodeGraph/Nodes/DataGraphNode.h"

#include "ConvertNodes.generated.h"

namespace Plu
{
	// Pin compatibility is an exact type-name compare (NodePin::CanConnect) — there are no implicit
	// conversions anywhere in the graph, by design. These two nodes are the explicit way across.

	PLU_STRUCT()
	struct PLU_API ConvertIntToFloatNode : ConvertGraphNode
	{
		REFLECTION_BODY_CONVERTINTTOFLOATNODE()

		static constexpr const char* ValuePin = "Value";

		PLU_PROPERTY()
		int Value = 0;

		String OperationName() override { return "Int to Float"; }

		void BuildPins() override
		{
			AddValueInput(ValuePin, EGraphValueType::Int);
			AddValueOutput(ResultPin, EGraphValueType::Float);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Float)) return false;
			*static_cast<float*>(outValue) = static_cast<float>(ReadDataPin<int>(context, ValuePin, Value));
			return true;
		}
	};

	// Truncates toward zero (plain C++ cast), matching what a static_cast in code would do — round
	// explicitly upstream with Add 0.5 / Clamp if that is not what you want.
	PLU_STRUCT()
	struct PLU_API ConvertFloatToIntNode : ConvertGraphNode
	{
		REFLECTION_BODY_CONVERTFLOATTOINTNODE()

		static constexpr const char* ValuePin = "Value";

		PLU_PROPERTY()
		float Value = 0.0f;

		String OperationName() override { return "Float to Int"; }

		void BuildPins() override
		{
			AddValueInput(ValuePin, EGraphValueType::Float);
			AddValueOutput(ResultPin, EGraphValueType::Int);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Int)) return false;
			*static_cast<int*>(outValue) = static_cast<int>(ReadDataPin<float>(context, ValuePin, Value));
			return true;
		}
	};
}

#endif //PLUENGINE_CONVERTNODES_H
