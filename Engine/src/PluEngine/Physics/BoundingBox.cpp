//
// Created by Plutex on 5/6/26.
//

#include "PluEngine/Physics/BoundingBox.h"

#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"

Plu::String Plu::BoundingBox::ToString()
{
    return String::Format("BoundingBox(MinX {0}, MaxX {1}, MinY {2}, MaxY {3}, MinZ {4}, MaxZ {5})",X.x, X.y, Y.x, Y.y, Z.x, Z.y);
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

    PLU_CORE_TRACE("BoundingBox(MinX {0}, MaxX {1}, MinY {2}, MaxY {3}, MinZ {4}, MaxZ {5})",X.x, X.y, Y.x, Y.y, Z.x, Z.y);
    return BoundingBox(X, Y, Z);
}
