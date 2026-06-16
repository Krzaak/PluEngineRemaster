//
// Created by Plutex on 4/14/26.
//

#include "PluEngine/BasicEngineClasses/Components/PhysicsSphereComponent.h"

#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "PluEngine/PluUtils.h"

Plu::PhysicsSphereComponent::PhysicsSphereComponent()
{
    SphereRadius = 0.5f;
}

Plu::PhysicsSphereComponent::~PhysicsSphereComponent()
{
}

JPH::ShapeRefC Plu::PhysicsSphereComponent::GetShape()
{
    float radius = Plu::ClampF(SphereRadius, 0.001f, FLT_MAX);
    JPH::ShapeRefC shape = new JPH::SphereShape(radius);
    return shape;
}
