//
// Created by Plutex on 2026-03-09.
//

#ifndef PLUENGINE_PHYSICSCAPSULECOMPONENT_H
#define PLUENGINE_PHYSICSCAPSULECOMPONENT_H
#include "PhysicsBodyComponent.h"
#include "PluEngine/Core.h"
#include "PhysicsCapsuleComponent.generated.h"

namespace Plu
{
	PLU_CLASS(PyExport)
	class PLU_API PhysicsCapsuleComponent : public PhysicsBodyComponent
	{
		REFLECTION_BODY_PHYSICSCAPSULECOMPONENT()
	public:
		PhysicsCapsuleComponent();
		virtual ~PhysicsCapsuleComponent() override;

		PLU_PROPERTY(PyExport)
		float CapsuleRadius;
		PLU_PROPERTY(PyExport)
		float CapsuleHalfHeight;

		JPH::ShapeRefC GetShape() override;
	};
}

#endif //PLUENGINE_PHYSICSBOXCOMPONENT_H