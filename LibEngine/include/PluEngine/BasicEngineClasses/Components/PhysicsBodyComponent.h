//
// Created by Plutex on 2026-03-08.
//

#ifndef PLUENGINE_PHYSICSBODYCOMPONENT_H
#define PLUENGINE_PHYSICSBODYCOMPONENT_H
#include "PluEngine/Core.h"
#include "PluEngine/GameObject/WorldComponent.h"
#include "PhysicsBodyComponent.generated.h"
#include "PluEngine/Physics/PhysicsBody.h"

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

		PLU_PROPERTY(PyExport)
		bool ActiveBody = false;

		PLU_PROPERTY(PyExport)
		PhysicsBodyMode BodyMode = PhysicsBodyMode::Solid;
	};
}

#endif //PLUENGINE_PHYSICSBODYCOMPONENT_H
