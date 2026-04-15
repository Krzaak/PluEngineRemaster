//
// Created by Plutex on 2026-03-26.
//

#include "PluEngine/BasicEngineClasses/Components/PhysicsCapsuleComponent.h"

#include "Jolt/Physics/Collision/Shape/CapsuleShape.h"
#include "PluEngine/Managers/ScenesManager.h"

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
	JPH::ShapeRefC shape = new JPH::CapsuleShape(CapsuleHalfHeight, CapsuleRadius);
	return shape;
}
