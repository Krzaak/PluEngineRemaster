//
// Created by Plutex on 2026-06-14.
//

#include "PluEngine/Physics/StaticMeshCollisionBuilder.h"
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include "PluEngine/Physics/BoundingBox.h"
#include "PluEngine/PluUtils.h"
#include "PluEngine/Log.h"

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
        DynamicArray<MeshCollisionShapeEntry> result;
        if (!mesh) return result;
        for (const auto& def : mesh->CollisionShapes)
        {
            MeshCollisionShapeEntry entry = BuildEntryFromDef(def, mesh, scale);
            if (entry.Shape) result.PushBack(entry);
        }
        return result;
    }
}
