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
        // Offset in mesh-local space, in Jolt sub-shape convention: pass it straight to
        // CompoundShapeSettings::AddShape. Jolt adds Shape::GetCenterOfMass() on top of it,
        // so shapes whose local origin is COM-shifted (ConvexHull) keep LocalOffset zero.
        // For direct debug drawing add Shape->GetCenterOfMass() yourself.
        Vec3 LocalOffset;
    };

    PLU_API DynamicArray<MeshCollisionShapeEntry> BuildCollisionShapesForMesh(StaticMesh* mesh, Vec3 scale = Vec3(1.0f));
}
