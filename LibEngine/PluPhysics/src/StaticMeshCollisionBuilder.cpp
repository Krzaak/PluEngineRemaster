//
// Created by Plutex on 2026-06-14.
//

#include "PluEngine/Physics/StaticMeshCollisionBuilder.h"
#include "PluEngine/Physics/PhysicsUtils.h"
#include "PluEngine/AssetTypes/MeshBounds.h"
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include "PluEngine/Core/BoundingBox.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Log.h"
#include "PluEngine/Timer.h"

namespace Plu
{
    static MeshCollisionShapeEntry BuildEntryFromDef(const StaticMeshCollisionDef& def, StaticMesh* mesh, Vec3 scale)
    {
        MeshCollisionShapeEntry entry;
        const auto& vertices = mesh->StaticMeshData.Vertices;
        const auto& indices  = mesh->StaticMeshData.Indices;

        if (def.Type == StaticMeshCollisionType::Approximate)
        {
            BoundingBox bb = CreateBoundingBoxForStaticMesh(mesh);
            Vec3 center = bb.GetCenter() * scale;
            // GetExtent() already returns half-extents (center to min distance)
            Vec3 halfExtent = bb.GetExtent() * scale;

            if (def.ApproxMode == ApproximateCollisionMode::BoundingBox)
            {
                halfExtent.x = Plu::ClampF(halfExtent.x, 0.001f, FLT_MAX);
                halfExtent.y = Plu::ClampF(halfExtent.y, 0.001f, FLT_MAX);
                halfExtent.z = Plu::ClampF(halfExtent.z, 0.001f, FLT_MAX);
                entry.Shape = new JPH::BoxShape(JPH::Vec3(halfExtent.x, halfExtent.y, halfExtent.z));
                entry.LocalOffset = center;
            }
            else if (def.ApproxMode == ApproximateCollisionMode::Sphere)
            {
                float radius = Plu::ClampF(glm::max(halfExtent.x, glm::max(halfExtent.y, halfExtent.z)), 0.001f, FLT_MAX);
                entry.Shape = new JPH::SphereShape(radius);
                entry.LocalOffset = center;
            }
            else // ConvexHull
            {
                if (vertices.IsEmpty()) return entry;
                JPH::ConvexHullShapeSettings settings;
                settings.mPoints.reserve(vertices.Size());
                for (UInt32 i = 0; i < vertices.Size(); i++)
                {
                    Vec3 p = vertices[i].Position * scale;
                    settings.mPoints.push_back(JPH::Vec3(p.x, p.y, p.z));
                }
                settings.mMaxConvexRadius = JPH::cDefaultConvexRadius;
                JPH::Shape::ShapeResult result = settings.Create();
                if (result.HasError())
                {
                    PLU_CORE_ERROR("ConvexHull shape creation failed: {}", result.GetError().c_str());
                    return entry;
                }
                entry.Shape = result.Get();
                // LocalOffset is in Jolt sub-shape space (see header): the hull was built from
                // mesh-space points, so it already sits where it should. Jolt shifts the hull
                // vertices by -COM internally and CompoundShape::SubShape::SetTransform adds
                // GetCenterOfMass() back — compensating here would double-count it.
                entry.LocalOffset = Vec3(0.0f);
            }
        }
        else // PerVertex
        {
            if (vertices.IsEmpty() || indices.IsEmpty()) return entry;
            if (indices.Size() % 3 != 0)
            {
                PLU_CORE_ERROR("PerVertex mesh has non-triangle index count!");
                return entry;
            }
            JPH::TriangleList triangles;
            triangles.reserve(indices.Size() / 3);
            for (UInt32 i = 0; i + 2 < indices.Size(); i += 3)
            {
                Vec3 a = vertices[indices[i+0]].Position * scale;
                Vec3 b = vertices[indices[i+1]].Position * scale;
                Vec3 c = vertices[indices[i+2]].Position * scale;
                triangles.push_back(JPH::Triangle(
                    JPH::Float3(a.x, a.y, a.z),
                    JPH::Float3(b.x, b.y, b.z),
                    JPH::Float3(c.x, c.y, c.z)
                ));
            }
            JPH::MeshShapeSettings settings(std::move(triangles));
            JPH::Shape::ShapeResult result = settings.Create();
            if (result.HasError())
            {
                PLU_CORE_ERROR("PerVertex mesh shape creation failed: {}", result.GetError().c_str());
                return entry;
            }
            entry.Shape = result.Get();
            entry.LocalOffset = Vec3(0.0f);
        }

        return entry;
    }

    DynamicArray<MeshCollisionShapeEntry> BuildCollisionShapesForMesh(StaticMesh* mesh, Vec3 scale)
    {
        PLU_PROFILE_SCOPE("Build Mesh Collision Shapes");
        DynamicArray<MeshCollisionShapeEntry> result;
        if (!mesh) return result;
        for (const auto& def : mesh->CollisionShapes)
        {
            MeshCollisionShapeEntry entry = BuildEntryFromDef(def, mesh, scale);
            if (entry.Shape) result.PushBack(entry);
        }
        return result;
    }

    const DynamicArray<MeshCollisionShapeEntry>* GetOrBuildUnscaledCollisionShapesForMesh(
        MeshCollisionShapeCache& cache, StaticMesh* mesh)
    {
        if (!mesh) return nullptr;

        if (const DynamicArray<MeshCollisionShapeEntry>* cached = cache.Find(mesh)) {
            return cached;
        }

        cache.Insert(mesh, BuildCollisionShapesForMesh(mesh));
        return cache.Find(mesh);
    }

    DynamicArray<MeshCollisionShapeEntry> GetOrBuildCollisionShapesForMesh(
        MeshCollisionShapeCache& cache, StaticMesh* mesh, Vec3 scale)
    {
        if (!mesh) return {};

        const DynamicArray<MeshCollisionShapeEntry>* cached = GetOrBuildUnscaledCollisionShapesForMesh(cache, mesh);
        if (!cached) return {};

        // Copied out before anything else can touch the cache: the ScaledShape wrapping below mutates
        // the entries, and they must not be written back into the shared unscaled set.
        DynamicArray<MeshCollisionShapeEntry> entries = *cached;
        if (scale == Vec3(1.0f)) return entries;

        for (auto& entry : entries)
        {
            if (!entry.Shape) continue;
            if (!entry.Shape->IsValidScale(ToJPHVec3(scale)))
            {
                // Jolt cannot express this scale for this shape (e.g. non-uniform on a sphere).
                // Building the whole mesh at the exact scale is what the uncached path always did,
                // so correctness is preserved — we just pay for it.
                return BuildCollisionShapesForMesh(mesh, scale);
            }
            entry.Shape = new JPH::ScaledShape(entry.Shape.GetPtr(), ToJPHVec3(scale));
            // LocalOffset is mesh-local, so it scales with the geometry. Shapes that keep it at zero
            // (ConvexHull, MeshShape — they are COM-relative) are unaffected by the multiply.
            entry.LocalOffset = entry.LocalOffset * scale;
        }
        return entries;
    }
}
