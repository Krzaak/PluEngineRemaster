//
// Created by Plutex on 2026-03-08.
//

#include "PluEngine/BasicEngineClasses/Components/PhysicsBodyComponent.h"

#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Physics/PhysicsBody.h"
#include "PluEngine/Physics/PhysicsWorld.h"
#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/PluUtils.h"

using namespace Plu;

PhysicsBodyComponent::PhysicsBodyComponent()
{
	ActiveBody = false;
}

Vec3 PhysicsBodyComponent::GetLinearVelocity()
{
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return Vec3(0);
	return ToGLMFromVec3(body->GetLinearVelocity());
}

void PhysicsBodyComponent::SetLinearVelocity(const Vec3& velocity)
{
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return;
	body->SetLinearVelocity(ToJPHVec3(velocity));
}

void PhysicsBodyComponent::AddLinearVelocity(const Vec3& velocity)
{
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return;
	body->AddLinearVelocity(ToJPHVec3(velocity));
}

Vec3 PhysicsBodyComponent::GetAngularVelocity()
{
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return Vec3(0);
	return ToGLMFromVec3(body->GetAngularVelocity());
}

void PhysicsBodyComponent::SetAngularVelocity(const Vec3& angularVelocity)
{
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return;
	body->SetAngularVelocity(ToJPHVec3(angularVelocity));
}

void PhysicsBodyComponent::AddForce(const Vec3& force)
{
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return;
	body->AddForce(ToJPHVec3(force));
}

void PhysicsBodyComponent::AddTorque(const Vec3& torque)
{
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return;
	body->AddTorque(ToJPHVec3(torque));
}

void PhysicsBodyComponent::AddImpulse(const Vec3& impulse)
{
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return;
	body->AddImpulse(ToJPHVec3(impulse));
}

void PhysicsBodyComponent::AddAngularImpulse(const Vec3& impulse)
{
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return;
	body->AddAngularImpulse(ToJPHVec3(impulse));
}

float PhysicsBodyComponent::GetFriction()
{
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return Friction;
	return body->GetFriction();
}

void PhysicsBodyComponent::SetFriction(float friction)
{
	Friction = friction;
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return;
	body->SetFriction(friction);
}

float PhysicsBodyComponent::GetRestitution()
{
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return Restitution;
	return body->GetRestitution();
}

void PhysicsBodyComponent::SetRestitution(float restitution)
{
	Restitution = restitution;
	auto body = GetParentGameObject()->GetPhysicsBody();
	if (!body) return;
	body->SetRestitution(restitution);
}

void PhysicsBodyComponent::SetCollisionProfile(const String& profileName)
{
	CollisionProfile.Name = profileName;

	GameObject* parent = GetParentGameObject().GetRaw();
	// Only rebuild when a body already exists (i.e. we're playing / spawned at runtime). In edit
	// mode the body is built at Play() and will resolve the profile name then.
	if (!parent || !parent->GetPhysicsBody()) return;
	TUsePointer<SceneWorld> world = GetWorld();
	if (!world) return;
	if (PhysicsWorld* physicsWorld = world->GetPhysicsWorld())
		physicsWorld->RebuildGameObjectBody(parent);
}

String PhysicsBodyComponent::GetCollisionProfile()
{
	return CollisionProfile.Name;
}