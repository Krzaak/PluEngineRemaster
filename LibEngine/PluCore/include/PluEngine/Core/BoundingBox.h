//
// Created by Plutex on 5/6/26.
//

#ifndef PLUENGINE_BOUNDINGBOX_H
#define PLUENGINE_BOUNDINGBOX_H

#include "PluEngine/Core.h"
#include "BoundingBox.generated.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
    struct StaticMesh;
    struct SkeletalMesh;
    PLU_STRUCT()
    struct PLUCORE_API BoundingBox
    {
        REFLECTION_BODY_BOUNDINGBOX()

        //Vec2's Min and Max

        PLU_PROPERTY()
        Vec2 X;

        PLU_PROPERTY()
        Vec2 Y;

        PLU_PROPERTY()
        Vec2 Z;

        [[nodiscard]] String ToString();
        [[nodiscard]] Vec3 GetCenter() const;
        [[nodiscard]] Vec3 GetExtent() const;

        [[nodiscard]] Vec3 FitCamera(Vec3 origin, Vec3 rot, Vec2 aspect, float FOV) const;

        [[nodiscard]] BoundingBox Add(const BoundingBox& other) const;
        [[nodiscard]] BoundingBox Multiply(Vec3 multiplier) const;
    };

    PLUCORE_API BoundingBox CreateBoundingBoxForStaticMesh(StaticMesh* staticMesh);

    // Bind-pose bounds of a skeletal mesh (mirror of the static version — walks every vertex,
    // so cache the result, never call it per frame). Animation moves vertices outside these
    // bounds, so inflate the result before using it for culling.
    PLUCORE_API BoundingBox CreateBoundingBoxForSkeletalMesh(SkeletalMesh* skeletalMesh);

    PLUCORE_API BoundingBox CreateBoundingBox(DynamicArray<Vec3> points);
}

#endif //PLUENGINE_BOUNDINGBOX_H
