//
// Created by Plutex on 2026-03-08.
//

#include "PluEngine/BasicEngineClasses/Components/PhysicsBodyComponent.h"

#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Physics/PhysicsBody.h"

Plu::PhysicsBodyComponent::PhysicsBodyComponent()
{
	ActiveBody = false;
}

Plu::PhysicsBodyComponent::~PhysicsBodyComponent()
{
	if (!mPhysicsBody) return;
	GetObjectManagerFromParent()->DestroyObject(*mPhysicsBody->GetEngineObjectHandle());
	mPhysicsBody = nullptr;
}

void Plu::PhysicsBodyComponent::OnSetupComponent()
{
	GetParentGameObject()->GetObjectEventDispatcher()->Subscribe("LocationChange", [this](void*) {
		mPhysicsBody->SetPosition(JPH::RVec3(GetParentGameObject()->GetObjectLocation().x, GetParentGameObject()->GetObjectLocation().y, GetParentGameObject()->GetObjectLocation().z));
	});
	GetParentGameObject()->GetObjectEventDispatcher()->Subscribe("RotationChange", [this](void*) {
		glm::quat q = glm::quat(glm::radians(GetParentGameObject()->GetObjectRotation()));
		mPhysicsBody->SetRotation(JPH::Quat(q.x,q.y,q.z,q.w));
	});
}

void Plu::PhysicsBodyComponent::SyncParentFromPhysics()
{
	auto rot = mPhysicsBody->GetRotation();
	auto loc = mPhysicsBody->GetPosition();

	GetParentGameObject()->SetObjectLocation(Vec3(loc.GetX(), loc.GetY(), loc.GetZ()));
	GetParentGameObject()->SetObjectRotation(glm::eulerAngles(glm::quat(rot.GetW(), rot.GetX(), rot.GetY(), rot.GetZ())));
}
