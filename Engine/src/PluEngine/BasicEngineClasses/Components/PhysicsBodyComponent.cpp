//
// Created by Plutex on 2026-03-08.
//

#include "PluEngine/BasicEngineClasses/Components/PhysicsBodyComponent.h"

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
}
