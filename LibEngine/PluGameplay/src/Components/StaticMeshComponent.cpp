//
// Created by Plutex on 1/19/26.
//

#include "PluEngine/Gameplay/Components/StaticMeshComponent.h"
#include "PluEngine/AssetTypes/MeshBounds.h"

#include "PluEngine/Gameplay/GameObject.h"

Plu::TUsePointer<Plu::StaticMesh> Plu::StaticMeshComponent::GetStaticMesh()
{
	return StaticMeshToDisplay;
}

void Plu::StaticMeshComponent::SetStaticMesh(TUsePointer<StaticMesh> staticMesh)
{
	StaticMeshToDisplay = staticMesh;
	MeshBoundingBoxComputed = false;
	if (staticMesh && staticMesh->IsLoaded) {
		MeshBoundingBox = Plu::CreateBoundingBoxForStaticMesh(staticMesh.GetRaw());
		MeshBoundingBoxComputed = true;
	}
	// The mesh's collision shapes are baked into the owning object's compound shape, so swapping the
	// mesh changes the collision geometry just like moving the component does. Unconditional: the
	// old mesh may have had collision even when the new one has none.
	MarkOwnerCollisionDirty();
}

void Plu::StaticMeshComponent::OnRelativeTransformChanged()
{
	if (!StaticMeshToDisplay || StaticMeshToDisplay->CollisionShapes.IsEmpty()) return;
	MarkOwnerCollisionDirty();
}

Plu::TUsePointer<Plu::MaterialInfo> Plu::StaticMeshComponent::GetMaterial()
{
	return Material;
}

void Plu::StaticMeshComponent::SetMaterial(TUsePointer<MaterialInfo> material)
{
	Material = material;
}

Matrix4 Plu::StaticMeshComponent::GetRenderMatrix()
{
	return GetWorldMatrix();
}
