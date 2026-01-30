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
}

Plu::ShaderProgram* Plu::StaticMeshComponent::GetShaderProgramToRender()
{
	return Shader.GetRaw();
}

Plu::StaticMesh* Plu::StaticMeshComponent::GetStaticMeshToRender()
{
	return StaticMeshToDisplay.GetRaw();
}

Vec3 Plu::StaticMeshComponent::GetRenderLocation()
{
	return GetParentGameObject()->GetObjectLocation();
}

Vec3 Plu::StaticMeshComponent::GetRenderRotation()
{
	return GetParentGameObject()->GetObjectRotation();
}

Vec3 Plu::StaticMeshComponent::GetRenderScale()
{
	return GetParentGameObject()->GetObjectScale();
}
