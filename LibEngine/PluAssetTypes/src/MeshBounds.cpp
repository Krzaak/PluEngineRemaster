//
// Created by Plutex on 8/12/26.
//

#include "PluEngine/AssetTypes/MeshBounds.h"

#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/AssetTypes/SkeletalMesh/SkeletalMesh.h"

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
