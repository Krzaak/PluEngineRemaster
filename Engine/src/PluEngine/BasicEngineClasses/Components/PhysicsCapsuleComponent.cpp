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

void Plu::PhysicsCapsuleComponent::CreatePhysicsBody()
{
	JPH::ShapeRefC shape = new JPH::CapsuleShape(CapsuleHalfHeight, CapsuleRadius);
	EngineObjectHandle bodyHDL = GetObjectManagerFromParent()->CreateObject<PhysicsBody>(
		GetWorld()->GetPhysicsWorld()->GetBodyInterface(),
		shape,
		JPH::RVec3(GetWorldLocation().x, GetWorldLocation().y, GetWorldLocation().z),
		ActiveBody ? BodyType::Dynamic : BodyType::Static
	);
	mPhysicsBody = GetObjectManagerFromParent()->GetObjectAsOwner<PhysicsBody>(bodyHDL);
}
