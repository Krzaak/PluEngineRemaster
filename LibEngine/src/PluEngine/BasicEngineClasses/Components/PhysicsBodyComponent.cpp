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
}

UInt64 PhysicsBodyComponent::MakeMaterialUserData()
{
	PhysicsMaterialData data;
	data.Friction = Friction;
	data.Restitution = Restitution;

	// Resolve the UE-style profile name to an index now, so the contact listener can compare
	// indices without a per-contact name lookup. Falls back to profile 0 ("Default") when the
	// world/profile is unavailable (FindProfileIndex returns 0 for unknown names).
	data.CollisionProfileIndex = 0;
	if (TUsePointer<SceneWorld> world = GetWorld())
		if (PhysicsWorld* physicsWorld = world->GetPhysicsWorld())
			data.CollisionProfileIndex = physicsWorld->ResolveCollisionProfileIndex(CollisionProfile.Name);

	return PackPhysicsMaterial(data);
}

// Rebuilds the owning game object's body so a friction/restitution edit takes effect: the value is
// baked into the shape's material, which is immutable after creation. Mirrors SetCollisionProfile.
// No-op in edit mode (no body yet) — the material is built fresh at Play().
void PhysicsBodyComponent::RebuildOwnerBody()
{
	GameObject* parent = GetParentGameObject().GetRaw();
	if (!parent || !parent->GetPhysicsBody()) return;
	TUsePointer<SceneWorld> world = GetWorld();
	if (!world) return;
	if (PhysicsWorld* physicsWorld = world->GetPhysicsWorld())
		physicsWorld->RebuildGameObjectBody(parent);
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
	// Friction is a per-sub-shape material value now (combined per-contact in the listener), not a
	// body property — return the authoritative component field.
	return Friction;
}

void PhysicsBodyComponent::SetFriction(float friction)
{
	Friction = friction;
	RebuildOwnerBody(); // bake the new value into the shape's material
}

float PhysicsBodyComponent::GetRestitution()
{
	return Restitution;
}

void PhysicsBodyComponent::SetRestitution(float restitution)
{
	Restitution = restitution;
	RebuildOwnerBody();
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