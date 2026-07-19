//
// Created by Plutex on 1/19/26.
//

#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"

#include "PluEngine/GameObject/GameObject.h"

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
