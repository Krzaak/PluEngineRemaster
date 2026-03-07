//
// Created by Plutex on 2026-03-07.
//

#ifndef PLUENGINE_PHYSICSBODY_H
#define PLUENGINE_PHYSICSBODY_H

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

namespace Plu
{
	enum class BodyType {
		Static,
		Dynamic,
		Kinematic
	};

	class PhysicsBody {
	public:
		PhysicsBody(
			JPH::BodyInterface& BodyInterface,
			JPH::ShapeRefC      Shape,
			const JPH::RVec3&   Position,
			BodyType           Type = BodyType::Static
		);
		~PhysicsBody();

		PhysicsBody(const PhysicsBody&) = delete;
		PhysicsBody& operator=(const PhysicsBody&) = delete;

		JPH::RVec3  GetPosition() const;
		JPH::Quat   GetRotation() const;
		JPH::BodyID GetID()       const { return mBodyID; }
		bool        IsValid()     const { return !mBodyID.IsInvalid(); }

		void SetPosition(const JPH::RVec3& Position);
		void SetRotation(const JPH::Quat&  Rotation);
		void AddForce(const JPH::Vec3&     Force);
		void AddImpulse(const JPH::Vec3&   Impulse);
		void SetLinearVelocity(const JPH::Vec3& Velocity);

	private:
		JPH::BodyInterface& mBodyInterface;
		JPH::BodyID         mBodyID;

		static JPH::EMotionType ToJoltMotionType(BodyType Type);
		static JPH::ObjectLayer ToJoltLayer(BodyType Type);
	};
}

#endif //PLUENGINE_PHYSICSBODY_H