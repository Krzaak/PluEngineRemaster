//
// Created by Plutex on 2026-06-14.
//

#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluSTL_FWD.h"

namespace Plu
{
    struct MeshCollisionShapeEntry
    {
        JPH::ShapeRefC Shape;
        Vec3 LocalOffset; // offset in mesh-local space needed to position the shape correctly
    };

    PLU_API DynamicArray<MeshCollisionShapeEntry> BuildCollisionShapesForMesh(StaticMesh* mesh, Vec3 scale = Vec3(1.0f));
}
