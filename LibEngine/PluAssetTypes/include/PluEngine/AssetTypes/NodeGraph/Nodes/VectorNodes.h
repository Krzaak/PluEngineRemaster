//
// Created by Plutex on 7/25/26.
//

#ifndef PLUENGINE_VECTORNODES_H
#define PLUENGINE_VECTORNODES_H

#include "PluEngine/Core.h"
#include "PluEngine/AssetTypes/NodeGraph/Nodes/DataGraphNode.h"
#include <glm/glm.hpp>

#include "VectorNodes.generated.h"

namespace Plu
{
	// Vector nodes have fixed pin types on purpose: their two sides differ (Vec3 in, float out for
	// Length/Dot/Distance), so there is nothing for the wildcard ValueType to switch.

	// Vec3 × float. The scalar counterpart of MathMultiplyNode, which is component-wise Vec3 × Vec3.
	PLU_STRUCT()
	struct PLUASSETTYPES_API VectorScaleNode : VectorGraphNode
	{
		REFLECTION_BODY_VECTORSCALENODE()

		static constexpr const char* VectorPin = "Vector";
		static constexpr const char* ScalePin  = "Scale";

		PLU_PROPERTY()
		GraphValue Vector;

		PLU_PROPERTY()
		float Scale = 1.0f;

		String OperationName() override { return "Scale"; }

		void BuildPins() override
		{
			Vector.Type = EGraphValueType::Vec3;
			AddValueInput(VectorPin, EGraphValueType::Vec3);
			AddValueInput(ScalePin, EGraphValueType::Float);
			AddValueOutput(ResultPin, EGraphValueType::Vec3);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Vec3)) return false;
			*static_cast<Vec3*>(outValue) = ReadDataPin<Vec3>(context, VectorPin, Vector.Vector)
			                              * ReadDataPin<float>(context, ScalePin, Scale);
			return true;
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API VectorLengthNode : VectorGraphNode
	{
		REFLECTION_BODY_VECTORLENGTHNODE()

		static constexpr const char* VectorPin = "Vector";

		PLU_PROPERTY()
		GraphValue Vector;

		String OperationName() override { return "Length"; }

		void BuildPins() override
		{
			Vector.Type = EGraphValueType::Vec3;
			AddValueInput(VectorPin, EGraphValueType::Vec3);
			AddValueOutput(ResultPin, EGraphValueType::Float);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Float)) return false;
			*static_cast<float*>(outValue) = glm::length(ReadDataPin<Vec3>(context, VectorPin, Vector.Vector));
			return true;
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API VectorNormalizeNode : VectorGraphNode
	{
		REFLECTION_BODY_VECTORNORMALIZENODE()

		static constexpr const char* VectorPin = "Vector";

		PLU_PROPERTY()
		GraphValue Vector;

		String OperationName() override { return "Normalize"; }

		void BuildPins() override
		{
			Vector.Type = EGraphValueType::Vec3;
			AddValueInput(VectorPin, EGraphValueType::Vec3);
			AddValueOutput(ResultPin, EGraphValueType::Vec3);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Vec3)) return false;
			const Vec3 vector = ReadDataPin<Vec3>(context, VectorPin, Vector.Vector);
			// glm::normalize of a zero vector is NaN — a zero-length input is a normal authoring
			// state (nothing wired yet), so hand back zero instead of poisoning everything downstream.
			const float length = glm::length(vector);
			*static_cast<Vec3*>(outValue) = length > 0.0f ? vector / length : Vec3(0.0f);
			return true;
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API VectorDotNode : VectorGraphNode
	{
		REFLECTION_BODY_VECTORDOTNODE()

		static constexpr const char* APin = "A";
		static constexpr const char* BPin = "B";

		PLU_PROPERTY()
		GraphValue A;

		PLU_PROPERTY()
		GraphValue B;

		String OperationName() override { return "Dot"; }

		void BuildPins() override
		{
			A.Type = EGraphValueType::Vec3;
			B.Type = EGraphValueType::Vec3;
			AddValueInput(APin, EGraphValueType::Vec3);
			AddValueInput(BPin, EGraphValueType::Vec3);
			AddValueOutput(ResultPin, EGraphValueType::Float);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Float)) return false;
			*static_cast<float*>(outValue) = glm::dot(ReadDataPin<Vec3>(context, APin, A.Vector),
			                                          ReadDataPin<Vec3>(context, BPin, B.Vector));
			return true;
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API VectorCrossNode : VectorGraphNode
	{
		REFLECTION_BODY_VECTORCROSSNODE()

		static constexpr const char* APin = "A";
		static constexpr const char* BPin = "B";

		PLU_PROPERTY()
		GraphValue A;

		PLU_PROPERTY()
		GraphValue B;

		String OperationName() override { return "Cross"; }

		void BuildPins() override
		{
			A.Type = EGraphValueType::Vec3;
			B.Type = EGraphValueType::Vec3;
			AddValueInput(APin, EGraphValueType::Vec3);
			AddValueInput(BPin, EGraphValueType::Vec3);
			AddValueOutput(ResultPin, EGraphValueType::Vec3);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Vec3)) return false;
			*static_cast<Vec3*>(outValue) = glm::cross(ReadDataPin<Vec3>(context, APin, A.Vector),
			                                           ReadDataPin<Vec3>(context, BPin, B.Vector));
			return true;
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API VectorDistanceNode : VectorGraphNode
	{
		REFLECTION_BODY_VECTORDISTANCENODE()

		static constexpr const char* APin = "A";
		static constexpr const char* BPin = "B";

		PLU_PROPERTY()
		GraphValue A;

		PLU_PROPERTY()
		GraphValue B;

		String OperationName() override { return "Distance"; }

		void BuildPins() override
		{
			A.Type = EGraphValueType::Vec3;
			B.Type = EGraphValueType::Vec3;
			AddValueInput(APin, EGraphValueType::Vec3);
			AddValueInput(BPin, EGraphValueType::Vec3);
			AddValueOutput(ResultPin, EGraphValueType::Float);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Float)) return false;
			*static_cast<float*>(outValue) = glm::distance(ReadDataPin<Vec3>(context, APin, A.Vector),
			                                               ReadDataPin<Vec3>(context, BPin, B.Vector));
			return true;
		}
	};

	PLU_STRUCT()
	struct PLUASSETTYPES_API VectorMakeNode : VectorGraphNode
	{
		REFLECTION_BODY_VECTORMAKENODE()

		static constexpr const char* XPin = "X";
		static constexpr const char* YPin = "Y";
		static constexpr const char* ZPin = "Z";

		PLU_PROPERTY()
		float X = 0.0f;

		PLU_PROPERTY()
		float Y = 0.0f;

		PLU_PROPERTY()
		float Z = 0.0f;

		String OperationName() override { return "Make Vec3"; }

		void BuildPins() override
		{
			AddValueInput(XPin, EGraphValueType::Float);
			AddValueInput(YPin, EGraphValueType::Float);
			AddValueInput(ZPin, EGraphValueType::Float);
			AddValueOutput(ResultPin, EGraphValueType::Vec3);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (!IsOutput(pinName, typeId, ResultPin, EGraphValueType::Vec3)) return false;
			*static_cast<Vec3*>(outValue) = Vec3(ReadDataPin<float>(context, XPin, X),
			                                     ReadDataPin<float>(context, YPin, Y),
			                                     ReadDataPin<float>(context, ZPin, Z));
			return true;
		}
	};

	// The only node with more than one output pin: each component is its own float output.
	PLU_STRUCT()
	struct PLUASSETTYPES_API VectorBreakNode : VectorGraphNode
	{
		REFLECTION_BODY_VECTORBREAKNODE()

		static constexpr const char* VectorPin = "Vector";
		static constexpr const char* XPin = "X";
		static constexpr const char* YPin = "Y";
		static constexpr const char* ZPin = "Z";

		PLU_PROPERTY()
		GraphValue Vector;

		String OperationName() override { return "Break Vec3"; }

		void BuildPins() override
		{
			Vector.Type = EGraphValueType::Vec3;
			AddValueInput(VectorPin, EGraphValueType::Vec3);
			AddValueOutput(XPin, EGraphValueType::Float);
			AddValueOutput(YPin, EGraphValueType::Float);
			AddValueOutput(ZPin, EGraphValueType::Float);
		}

		bool EvaluateDataOutput(GraphEvalContext& context, const String& pinName,
		                        const String& typeId, void* outValue) override
		{
			if (typeId != GraphValue::PinTypeId(EGraphValueType::Float)) return false;

			const Vec3 vector = ReadDataPin<Vec3>(context, VectorPin, Vector.Vector);
			if (pinName == XPin)      { *static_cast<float*>(outValue) = vector.x; return true; }
			else if (pinName == YPin) { *static_cast<float*>(outValue) = vector.y; return true; }
			else if (pinName == ZPin) { *static_cast<float*>(outValue) = vector.z; return true; }
			return false;
		}
	};
}

#endif //PLUENGINE_VECTORNODES_H
