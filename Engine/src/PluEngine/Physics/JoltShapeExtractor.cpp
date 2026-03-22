//
// Created by Plutex on 2026-03-09.
//

#include "PluEngine/Physics/JoltShapeExtractor.h"

using namespace Plu;

DynamicArray<glm::vec3> JoltShapeExtractor::ExtractTriangles(const JPH::Shape* shape)
{
	DynamicArray<glm::vec3> verts;

	JPH::Shape::GetTrianglesContext ctx;
	shape->GetTrianglesStart(
		ctx,
		JPH::AABox::sBiggest(),
		JPH::Vec3::sZero(),
		JPH::Quat::sIdentity(),
		JPH::Vec3::sReplicate(1.0f)
	);

	constexpr int kBatch = 512;
	JPH::Float3 buf[kBatch * 3];
	int count;

	while ((count = shape->GetTrianglesNext(ctx, kBatch, buf)) > 0)
	{
		for (int i = 0; i < count * 3; i++)
			verts.PushBack({ buf[i].x, buf[i].y, buf[i].z });
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