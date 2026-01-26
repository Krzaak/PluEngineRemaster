//
// Created by Plutex on 1/19/26.
//

#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"

Plu::TUsePointer<Plu::StaticMesh> Plu::StaticMeshComponent::GetStaticMesh()
{
	return mStaticMesh;
}

void Plu::StaticMeshComponent::SetStaticMesh(TUsePointer<StaticMesh> staticMesh)
{
	mStaticMesh = staticMesh;
}

Plu::TUsePointer<Plu::ShaderProgram>& Plu::StaticMeshComponent::GetShaderProgramToRender()
{
	return mShader;
}

Plu::TUsePointer<Plu::StaticMesh>& Plu::StaticMeshComponent::GetStaticMeshToRender()
{
	return mStaticMesh;
}
