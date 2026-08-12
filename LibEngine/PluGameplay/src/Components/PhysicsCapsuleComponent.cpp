//
// Created by Plutex on 2026-03-26.
//

#include "PluEngine/Gameplay/Components/PhysicsCapsuleComponent.h"

#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "PluEngine/Gameplay/Scenes/ScenesManager.h"
#include "PluEngine/PluUtils.h"

Plu::PhysicsCapsuleComponent::PhysicsCapsuleComponent()
{
	CapsuleHalfHeight = 1.0f;
	CapsuleRadius = 0.5f;
}

Plu::PhysicsCapsuleComponent::~PhysicsCapsuleComponent()
{
}

JPH::ShapeRefC Plu::PhysicsCapsuleComponent::GetShape()
{
	float halfHeight = Plu::ClampF(CapsuleHalfHeight, 0.001f, FLT_MAX);
	float radius     = Plu::ClampF(CapsuleRadius,     0.001f, FLT_MAX);
	JPH::Ref<JPH::CapsuleShape> shape = new JPH::CapsuleShape(halfHeight, radius);
	shape->SetUserData(MakeMaterialUserData()); // per-sub-shape friction/restitution/channel
	return JPH::ShapeRefC(shape.GetPtr());
}
