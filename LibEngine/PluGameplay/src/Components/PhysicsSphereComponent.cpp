//
// Created by Plutex on 4/14/26.
//

#include "PluEngine/Gameplay/Components/PhysicsSphereComponent.h"

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
    JPH::Ref<JPH::SphereShape> shape = new JPH::SphereShape(radius);
    shape->SetUserData(MakeMaterialUserData()); // per-sub-shape friction/restitution/channel
    return JPH::ShapeRefC(shape.GetPtr());
}
