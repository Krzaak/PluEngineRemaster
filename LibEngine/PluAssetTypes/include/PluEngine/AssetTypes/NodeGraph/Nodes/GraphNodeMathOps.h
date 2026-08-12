//
// Created by Plutex on 7/25/26.
//

#ifndef PLUENGINE_GRAPHNODEMATHOPS_H
#define PLUENGINE_GRAPHNODEMATHOPS_H

#include "PluEngine/PluTypes.h"
#include <glm/glm.hpp>

// Operation functors for the wildcard value nodes. Each one overloads operator() for every value type
// the node family supports, so DataGraphNode::EvalBinary/EvalUnary can pick the right arithmetic from
// the node's runtime ValueType without every node repeating the same switch.
//
// Deliberately NOT reflected (no PLU_* macros in this file): these are implementation details of the
// nodes, never authored, never serialized.
namespace Plu::GraphOps
{
	struct Add
	{
		float operator()(float a, float b) const { return a + b; }
		int   operator()(int a, int b) const { return a + b; }
		Vec3  operator()(const Vec3& a, const Vec3& b) const { return a + b; }
	};

	struct Subtract
	{
		float operator()(float a, float b) const { return a - b; }
		int   operator()(int a, int b) const { return a - b; }
		Vec3  operator()(const Vec3& a, const Vec3& b) const { return a - b; }
	};

	// Vec3 multiply/divide are component-wise (Hadamard). Scaling a vector by a scalar is a separate
	// node (VectorScaleNode) — the pin types differ, so it can't be a wildcard case here.
	struct Multiply
	{
		float operator()(float a, float b) const { return a * b; }
		int   operator()(int a, int b) const { return a * b; }
		Vec3  operator()(const Vec3& a, const Vec3& b) const { return a * b; }
	};

	// Division by zero yields 0 for that component instead of inf/NaN or a per-frame log: a half-wired
	// graph is a normal authoring state, and a NaN would silently poison a whole pose downstream.
	struct Divide
	{
		float operator()(float a, float b) const { return b == 0.0f ? 0.0f : a / b; }
		int   operator()(int a, int b) const { return b == 0 ? 0 : a / b; }
		Vec3  operator()(const Vec3& a, const Vec3& b) const
		{
			return Vec3(b.x == 0.0f ? 0.0f : a.x / b.x,
			            b.y == 0.0f ? 0.0f : a.y / b.y,
			            b.z == 0.0f ? 0.0f : a.z / b.z);
		}
	};

	struct Min
	{
		float operator()(float a, float b) const { return glm::min(a, b); }
		int   operator()(int a, int b) const { return glm::min(a, b); }
		Vec3  operator()(const Vec3& a, const Vec3& b) const { return glm::min(a, b); }
	};

	struct Max
	{
		float operator()(float a, float b) const { return glm::max(a, b); }
		int   operator()(int a, int b) const { return glm::max(a, b); }
		Vec3  operator()(const Vec3& a, const Vec3& b) const { return glm::max(a, b); }
	};

	struct Abs
	{
		float operator()(float value) const { return glm::abs(value); }
		int   operator()(int value) const { return glm::abs(value); }
		Vec3  operator()(const Vec3& value) const { return glm::abs(value); }
	};

	struct Negate
	{
		float operator()(float value) const { return -value; }
		int   operator()(int value) const { return -value; }
		Vec3  operator()(const Vec3& value) const { return -value; }
	};

	// Comparison functors — operands of the same type in, bool out (LogicCompareNode).
	struct Greater
	{
		bool operator()(float a, float b) const { return a > b; }
		bool operator()(int a, int b) const { return a > b; }
	};

	struct Less
	{
		bool operator()(float a, float b) const { return a < b; }
		bool operator()(int a, int b) const { return a < b; }
	};

	// Float equality goes through a small epsilon: exact == on a value that came out of a blend or a
	// lerp is a coin flip, and an equality node that never fires is worse than useless.
	struct Equal
	{
		bool operator()(float a, float b) const { return glm::abs(a - b) <= 0.0001f; }
		bool operator()(int a, int b) const { return a == b; }
	};
}

#endif //PLUENGINE_GRAPHNODEMATHOPS_H
