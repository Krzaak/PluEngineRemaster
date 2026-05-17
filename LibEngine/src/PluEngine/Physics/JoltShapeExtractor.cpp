//
// Created by Plutex on 2026-03-09.
//

#include "PluEngine/Physics/JoltShapeExtractor.h"

#include <Jolt/Physics/Collision/Shape/CompoundShape.h>

using namespace Plu;

DynamicArray<glm::vec3> JoltShapeExtractor::ExtractTriangles(const JPH::Shape* shape)
{
	DynamicArray<glm::vec3> verts;

	JPH::Shape::GetTrianglesContext ctx;
	if (shape->GetType() == JPH::EShapeType::Compound) {
		const JPH::CompoundShape* compound = static_cast<const JPH::CompoundShape*>(shape);

    for (uint32_t i = 0; i < compound->GetNumSubShapes(); ++i) {
        const JPH::CompoundShape::SubShape& sub = compound->GetSubShape(i);

        // Pobieramy trójkąty dla sub-shape'u
        JPH::Shape::GetTrianglesContext subCtx;
        sub.mShape->GetTrianglesStart(
            subCtx,
            JPH::AABox::sBiggest(),
            JPH::Vec3(sub.mPositionCOM),
			JPH::Quat::sEulerAngles(JPH::Vec3(sub.mRotation)),
            JPH::Vec3::sReplicate(1.0f) // Skalowanie (zazwyczaj 1.0, chyba że wspierasz non-uniform)
        );

        constexpr int kBatch = 512;
        JPH::Float3 buf[kBatch * 3];
        int count;

        while ((count = sub.mShape->GetTrianglesNext(subCtx, kBatch, buf)) > 0) {
            for (int j = 0; j < count * 3; j++) {
                verts.PushBack({ buf[j].x, buf[j].y, buf[j].z });
            }
        }
    }
	} else {
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