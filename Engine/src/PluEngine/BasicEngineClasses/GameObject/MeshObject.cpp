//
// Created by Plutex on 2026-03-18.
//

#include "PluEngine/BasicEngineClasses/GameObjects/MeshObject.h"

#include "PluEngine/BasicEngineClasses/Components/PhysicsBoxComponent.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"

void Plu::MeshObject::OnSetupComponents()
{
	MeshComponent = AddComponent(StaticMeshComponent::GetStaticClass(), "Mesh");
	BoxComponent = AddComponent(PhysicsBoxComponent::GetStaticClass(), "Box");
	BoxComponent->AttachTo(MeshComponent.GetRaw());
}
