//
// Created by Plutex on 2026-03-18.
//

#include "PluEngine/Gameplay/Objects/MeshObject.h"

#include "PluEngine/Gameplay/Components/PhysicsBoxComponent.h"
#include "PluEngine/Gameplay/Components/StaticMeshComponent.h"

void Plu::MeshObject::OnSetupComponents()
{
	MeshComponent = AddComponent(StaticMeshComponent::GetStaticClass(), "Mesh");
	BoxComponent = AddComponent(PhysicsBoxComponent::GetStaticClass(), "Box");
	BoxComponent->AttachTo(MeshComponent.GetRaw());
}
