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
    PLU_STRUCT()
    struct PLU_API BoundingBox
    {
        REFLECTION_BODY_BOUNDINGBOX()

        //Vec2's Min and Max

        PLU_PROPERTY()
        Vec2 X;

        PLU_PROPERTY()
        Vec2 Y;

        PLU_PROPERTY()
        Vec2 Z;

        String ToString();
    };

    PLU_API BoundingBox CreateBoundingBoxForStaticMesh(StaticMesh* staticMesh);
    PLU_API BoundingBox CreateBoundingBox(DynamicArray<Vec3> points);
}

#endif //PLUENGINE_BOUNDINGBOX_H
