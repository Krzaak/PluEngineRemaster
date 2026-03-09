//
// Created by Plutex on 2026-03-09.
//

#include "PluEngine/BasicEngineClasses/Components/PhysicsBoxComponent.h"

#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

Plu::PhysicsBoxComponent::PhysicsBoxComponent()
{
	BoxSize = {1,1,1};
}

Plu::PhysicsBoxComponent::~PhysicsBoxComponent()
{
}

void Plu::PhysicsBoxComponent::CreatePhysicsBody()
{
	JPH::ShapeRefC shape = new JPH::BoxShape(JPH::Vec3(BoxSize.x, BoxSize.y, BoxSize.z));
	EngineObjectHandle bodyHDL = GetObjectManagerFromParent()->CreateObject<PhysicsBody>(
		GetWorld()->GetPhysicsWorld()->GetBodyInterface(),
		shape,
		JPH::RVec3(GetWorldLocation().x, GetWorldLocation().y, GetWorldLocation().z),
		PhysicsBodyType
	);
	mPhysicsBody = GetObjectManagerFromParent()->GetObjectAsOwner<PhysicsBody>(bodyHDL);
}
