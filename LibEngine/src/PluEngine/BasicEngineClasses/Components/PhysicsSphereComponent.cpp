//
// Created by Plutex on 4/14/26.
//

#include "PluEngine/BasicEngineClasses/Components/PhysicsSphereComponent.h"

#include "Jolt/Physics/Collision/Shape/SphereShape.h"

Plu::PhysicsSphereComponent::PhysicsSphereComponent()
{
    SphereRadius = 0.5f;
}

Plu::PhysicsSphereComponent::~PhysicsSphereComponent()
{
}

JPH::ShapeRefC Plu::PhysicsSphereComponent::GetShape()
{
    JPH::ShapeRefC shape = new JPH::SphereShape(SphereRadius);
    return shape;
}
