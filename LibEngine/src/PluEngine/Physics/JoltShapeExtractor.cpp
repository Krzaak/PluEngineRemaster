//
// Created by Plutex on 2026-03-09.
//

#include "PluEngine/Physics/JoltShapeExtractor.h"

#include <Jolt/Physics/Collision/Shape/CompoundShape.h>
#include <Jolt/Physics/Collision/CollisionCollector.h>
#include <Jolt/Physics/Collision/TransformedShape.h>

using namespace Plu;

struct LeafCollector : public JPH::TransformedShapeCollector
{
	DynamicArray<JPH::TransformedShape> mLeaves;
	void AddHit(const JPH::TransformedShape& inResult) override { mLeaves.PushBack(inResult); }
};

DynamicArray<glm::vec3> JoltShapeExtractor::ExtractTriangles(const JPH::Shape* shape)
{
	DynamicArray<glm::vec3> verts;

	LeafCollector collector;
	JPH::TransformedShape ts(JPH::RVec3::sZero(), JPH::Quat::sIdentity(), shape, JPH::BodyID());
	ts.CollectTransformedShapes(JPH::AABox::sBiggest(), collector);

	for (int i = 0; i < collector.mLeaves.Size(); i++)
	{
		const JPH::TransformedShape& leaf = collector.mLeaves[i];
		JPH::Shape::GetTrianglesContext ctx;
		leaf.GetTrianglesStart(ctx, JPH::AABox::sBiggest(), JPH::RVec3::sZero());

		constexpr int kBatch = 512;
		JPH::Float3 buf[kBatch * 3];
		int count;
		while ((count = leaf.GetTrianglesNext(ctx, kBatch, buf)) > 0)
		{
			for (int j = 0; j < count * 3; j++)
				verts.PushBack({ buf[j].x, buf[j].y, buf[j].z });
		}
	}

	return verts;
}

glm::mat4 JoltShapeExtractor::JoltToGlm(const JPH::RMat44& m)
{
	glm::mat4 out(1.0f);
	for (int col = 0; col < 4; col++)
	{
		JPH::Vec4 c = m.GetColumn4(col);
		out[col] = { c.GetX(), c.GetY(), c.GetZ(), c.GetW() };
	}
	return out;
}
