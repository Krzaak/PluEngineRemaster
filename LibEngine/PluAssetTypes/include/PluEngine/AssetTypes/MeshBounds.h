//
// Created by Plutex on 8/12/26.
//

#ifndef PLUENGINE_MESHBOUNDS_H
#define PLUENGINE_MESHBOUNDS_H
#include "PluEngine/Core.h"
#include "PluEngine/Core/BoundingBox.h"

namespace Plu
{
    struct StaticMesh;
    struct SkeletalMesh;

    // Bounds of a mesh's vertices, in local space. Separate from BoundingBox itself, which is
    // plain geometry and knows nothing about assets: these two walk actual mesh data, so they
    // belong with the mesh types rather than with the box.
    //
    // Both walk every vertex — cache the result, never call them per frame.
    PLUASSETTYPES_API BoundingBox CreateBoundingBoxForStaticMesh(StaticMesh* staticMesh);

    // Bind-pose bounds of a skeletal mesh. Animation moves vertices outside these bounds, so
    // inflate the result before using it for culling.
    PLUASSETTYPES_API BoundingBox CreateBoundingBoxForSkeletalMesh(SkeletalMesh* skeletalMesh);
}

#endif //PLUENGINE_MESHBOUNDS_H
