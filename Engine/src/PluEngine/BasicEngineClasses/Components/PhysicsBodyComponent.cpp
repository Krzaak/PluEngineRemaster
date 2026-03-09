//
// Created by Plutex on 2026-03-08.
//

#include "PluEngine/BasicEngineClasses/Components/PhysicsBodyComponent.h"

#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Physics/PhysicsBody.h"

Plu::PhysicsBodyComponent::PhysicsBodyComponent()
{
	PhysicsBodyType = BodyType::Static;
}

Plu::PhysicsBodyComponent::~PhysicsBodyComponent()
{
	if (!mPhysicsBody) return;
	GetObjectManagerFromParent()->DestroyObject(*mPhysicsBody->GetEngineObjectHandle());
	mPhysicsBody = nullptr;

	mPhysicsBody->
}

void Plu::PhysicsBodyComponent::SyncParentFromPhysics()
{
	auto rot = mPhysicsBody->GetRotation();
	auto loc = mPhysicsBody->GetPosition();

	GetParentGameObject()->SetObjectLocation(Vec3(loc.GetX(), loc.GetY(), loc.GetZ()));
	GetParentGameObject()->SetObjectRotation(glm::eulerAngles(glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ())));
}
