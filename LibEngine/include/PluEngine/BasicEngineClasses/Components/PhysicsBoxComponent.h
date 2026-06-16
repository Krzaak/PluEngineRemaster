//
// Created by Plutex on 2026-03-09.
//

#ifndef PLUENGINE_PHYSICSBOXCOMPONENT_H
#define PLUENGINE_PHYSICSBOXCOMPONENT_H
#include "PhysicsBodyComponent.h"
#include "PluEngine/Core.h"
#include "PhysicsBoxComponent.generated.h"

namespace Plu
{
	PLU_CLASS(PyExport)
	class PLU_API PhysicsBoxComponent : public PhysicsBodyComponent
	{
		REFLECTION_BODY_PHYSICSBOXCOMPONENT()
	public:
		PhysicsBoxComponent();
		virtual ~PhysicsBoxComponent() override;

		PLU_PROPERTY(PyExport)
		Vec3 BoxSize;

		JPH::ShapeRefC GetShape() override;
	};
}

#endif //PLUENGINE_PHYSICSBOXCOMPONENT_H