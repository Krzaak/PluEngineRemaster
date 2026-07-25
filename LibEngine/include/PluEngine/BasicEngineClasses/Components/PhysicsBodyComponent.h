//
// Created by Plutex on 2026-03-08.
//

#ifndef PLUENGINE_PHYSICSBODYCOMPONENT_H
#define PLUENGINE_PHYSICSBODYCOMPONENT_H
#include "PluEngine/Core.h"
#include "PluEngine/GameObject/WorldComponent.h"
#include "PhysicsBodyComponent.generated.h"
#include "PluEngine/Physics/PhysicsBody.h"
#include "PluEngine/Physics/CollisionChannels.h"
#include "PluEngine/Physics/PluPhysicsMaterial.h"

namespace Plu
{
	PLU_CLASS(Abstract, PyExport)
	class PLU_API PhysicsBodyComponent : public WorldComponent
	{
		REFLECTION_BODY_PHYSICSBODYCOMPONENT()
	public:
		PhysicsBodyComponent();
		virtual ~PhysicsBodyComponent() override = default;

		virtual JPH::ShapeRefC GetShape() = 0;

	protected:
		// Packs this component's Friction/Restitution and resolved collision profile into a value to
		// store in the leaf shape's user data (Shape::SetUserData). See PluPhysicsMaterial.h.
		UInt64 MakeMaterialUserData();

		// Rebuilds the owning game object's body so a baked-in material change takes effect.
		void RebuildOwnerBody();

		// This component's relative transform is baked into the compound shape as its sub-shape
		// offset, so any change to it has to be pushed into the body.
		void OnRelativeTransformChanged() override;
	public:

		PLU_FUNCTION(PyExport)
		Vec3 GetLinearVelocity();
		PLU_FUNCTION(PyExport)
		void SetLinearVelocity(const Vec3& velocity);
		PLU_FUNCTION(PyExport)
		void AddLinearVelocity(const Vec3& velocity);

		PLU_FUNCTION(PyExport)
		Vec3 GetAngularVelocity();
		PLU_FUNCTION(PyExport)
		void SetAngularVelocity(const Vec3& angularVelocity);

		PLU_FUNCTION(PyExport)
		void AddForce(const Vec3& force);
		PLU_FUNCTION(PyExport)
		void AddTorque(const Vec3& torque);
		PLU_FUNCTION(PyExport)
		void AddImpulse(const Vec3& impulse);
		PLU_FUNCTION(PyExport)
		void AddAngularImpulse(const Vec3& impulse);

		PLU_FUNCTION(PyExport)
		float GetFriction();
		PLU_FUNCTION(PyExport)
		void SetFriction(float friction);

		PLU_FUNCTION(PyExport)
		float GetRestitution();
		PLU_FUNCTION(PyExport)
		void SetRestitution(float restitution);

		// Set the UE-style collision preset by name (see the generated CollisionProfiles class in
		// the project's <ProjectName>.py). Needed for components spawned from Python, which never
		// run scene deserialization and would otherwise keep the default "Default" preset. If a
		// body already exists (runtime), it is rebuilt so the new profile takes effect immediately.
		PLU_FUNCTION(PyExport)
		void SetCollisionProfile(const String& profileName);
		PLU_FUNCTION(PyExport)
		String GetCollisionProfile();

		// UE-style collision preset; resolved against the project's CollisionConfig at body
		// build time (PhysicsWorld::ResolveCollisionProfileIndex). Edited via a preset dropdown
		// in the editor; unknown names fall back to profile 0 ("Default").
		// (Not PyExport: CollisionProfileRef has no pybind caster — exposing would break bindings.)
		PLU_PROPERTY()
		CollisionProfileRef CollisionProfile;

		PLU_PROPERTY(PyExport)
		float Friction = 0.2f;

		PLU_PROPERTY(PyExport)
		float Restitution = 0.0f;
	};
}

#endif //PLUENGINE_PHYSICSBODYCOMPONENT_H
