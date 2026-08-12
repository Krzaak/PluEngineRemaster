//
// Created by Plutex on 5/6/26.
//

#include "PluEngine/Core/BoundingBox.h"

#include "PluEngine/PluUtils.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"

Plu::String Plu::BoundingBox::ToString()
{
    return String::Format("BoundingBox(MinX {0}, MaxX {1}, MinY {2}, MaxY {3}, MinZ {4}, MaxZ {5})",X.x, X.y, Y.x, Y.y, Z.x, Z.y);
}

Vec3 Plu::BoundingBox::GetCenter() const
{
    float centerX = (X.x + X.y) / 2;
    float centerY = (Y.x + Y.y) / 2;
    float centerZ = (Z.x + Z.y) / 2;
    return {centerX, centerY, centerZ};
}

Vec3 Plu::BoundingBox::GetExtent() const
{
    const Vec3 center = GetCenter();
    float extentX = center.x - X.x;
    float extentY = center.y - Y.x;
    float extentZ = center.z - Z.x;
    return {abs(extentX), abs(extentY), abs(extentZ)};
}

Vec3 Plu::BoundingBox::FitCamera(Vec3 origin, Vec3 rot, Vec2 aspect, float FOV) const
{
    Vec3 center   = this->GetCenter();
    Vec3 extent   = this->GetExtent();

    float radius   = glm::max(extent.x, glm::max(extent.y, extent.z));
    float fovX     = 2.0f * glm::atan(glm::tan(FOV * 0.5f) * aspect.x / aspect.y);
    float minFov   = glm::min(fovX, FOV);
    float distance = radius / glm::sin(minFov * 0.5f);

    // Kamera patrzy od przodu wzdłuż -Z
    Vec3 cameraPos = center + (GetForwardVector(rot) * distance);
    return cameraPos + origin;
}

Plu::BoundingBox Plu::BoundingBox::Add(const BoundingBox &other) const
{
    BoundingBox newBox = *this;
    if (other.X.x < this->X.x) newBox.X.x = other.X.x;
    if (other.X.y > this->X.y) newBox.X.y = other.X.y;

    if (other.Y.x < this->Y.x) newBox.Y.x = other.Y.x;
    if (other.Y.y > this->Y.y) newBox.Y.y = other.Y.y;

    if (other.Z.x < this->Z.x) newBox.Z.x = other.Z.x;
    if (other.Z.y > this->Z.y) newBox.Z.y = other.Z.y;

    return newBox;
}

Plu::BoundingBox Plu::BoundingBox::Multiply(Vec3 multiplier) const
{
    BoundingBox newBox = *this;
    newBox.X *= multiplier.x;
    newBox.Y *= multiplier.y;
    newBox.Z *= multiplier.z;
    return newBox;
}

Plu::BoundingBox Plu::CreateBoundingBoxForStaticMesh(StaticMesh* staticMesh)
{
    if (!staticMesh) return BoundingBox();
    DynamicArray<Vec3> points;
    DynamicArray<Vertex>* vertices = &staticMesh->StaticMeshData.Vertices;
    for (int i = 0; i < vertices->Size(); i++) {
        if (!staticMesh->StaticMeshData.Indices.Contains(i)) continue;
        Vertex vertex = vertices->At(i);
        points.PushBack(vertex.Position);
    }
    return CreateBoundingBox(points);
}

Plu::BoundingBox Plu::CreateBoundingBoxForSkeletalMesh(SkeletalMesh* skeletalMesh)
{
    if (!skeletalMesh) return BoundingBox();
    DynamicArray<Vec3> points;
    DynamicArray<SkeletalVertex>* vertices = &skeletalMesh->MeshData.Vertices;
    points.Reserve(vertices->Size());
    for (UInt32 i = 0; i < vertices->Size(); i++) {
        points.PushBack(vertices->At(i).Position);
    }
    return CreateBoundingBox(points);
}

Plu::BoundingBox Plu::CreateBoundingBox(DynamicArray<Vec3> points)
{
    Vec2 X = Vec2(0.0f);
    Vec2 Y = Vec2(0.0f);
    Vec2 Z = Vec2(0.0f);

    for (int i = 0; i < points.Size(); i++) {
        Vec3 point = points[i];

        //X
        if (point.x < X.x) X.x = point.x;
        if (point.x > X.y) X.y = point.x;

        //Y
        if (point.y < Y.x) Y.x = point.y;
        if (point.y > Y.y) Y.y = point.y;

        //Z
        if (point.z < Z.x) Z.x = point.z;
        if (point.z > Z.y) Z.y = point.z;

    }
    
    return BoundingBox(X, Y, Z);
}
