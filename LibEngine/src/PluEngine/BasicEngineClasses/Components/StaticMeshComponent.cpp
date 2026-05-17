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
	PLU_CORE_TRACE("New Mesh");
}

Plu::TUsePointer<Plu::MaterialInfo> Plu::StaticMeshComponent::GetMaterial()
{
	return Material;
}

void Plu::StaticMeshComponent::SetMaterial(TUsePointer<MaterialInfo> material)
{
	Material = material;
}

Plu::MaterialInfo * Plu::StaticMeshComponent::GetMaterialInfoToRender()
{
	return Material.GetRaw();
}

Plu::StaticMesh* Plu::StaticMeshComponent::GetStaticMeshToRender()
{
	return StaticMeshToDisplay.GetRaw();
}

Plu::EngineObjectHandle * Plu::StaticMeshComponent::GetRenderableObjectHandle()
{
	return GetEngineObjectHandle();
}

Matrix4 Plu::StaticMeshComponent::GetRenderMatrix()
{
	return GetWorldMatrix();
}
