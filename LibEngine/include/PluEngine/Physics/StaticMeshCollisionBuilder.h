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

    // Cache of **unscaled** collision geometry, one entry per mesh asset. Scale is deliberately not
    // part of the key: object scales are per-instance and in practice almost all distinct (a scene
    // of 1000 scattered props had 1002 distinct scales), so a scale-keyed cache never hits. Scale is
    // applied at use time by wrapping in JPH::ScaledShape instead — the same trick this codebase
    // already uses for PhysicsBodyComponent sub-shapes.
    using MeshCollisionShapeCache = GameHashMap<StaticMesh*, DynamicArray<MeshCollisionShapeEntry>>;

    // Unscaled shapes for a mesh, built once and memoised. Building is expensive — a ConvexHull over
    // every vertex or a MeshShape BVH over every triangle.
    //
    // Returns a pointer **into the cache** (null only for a null mesh). It is invalidated by the next
    // insertion into the same cache, so consume it before looking anything else up — do not store it
    // across frames or across further calls. Callers that need to keep the entries around should copy
    // what they need; a copy is cheap (refcount bumps on JPH::ShapeRefC) but pointless per frame,
    // which is exactly why this hands back a pointer.
    PLU_API const DynamicArray<MeshCollisionShapeEntry>* GetOrBuildUnscaledCollisionShapesForMesh(
        MeshCollisionShapeCache& cache, StaticMesh* mesh);

    // As above, then scaled: each shape is wrapped in a JPH::ScaledShape and its LocalOffset scaled
    // with it. Shapes that cannot represent the requested scale (Jolt only allows uniform scale on a
    // sphere, for instance) fall back to a direct build at that exact scale, so the geometry is
    // always correct — only the caching opportunity is lost.
    PLU_API DynamicArray<MeshCollisionShapeEntry> GetOrBuildCollisionShapesForMesh(
        MeshCollisionShapeCache& cache, StaticMesh* mesh, Vec3 scale = Vec3(1.0f));
}
