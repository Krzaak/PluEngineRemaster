//
// Created by Plutex on 2026-03-09.
//

#ifndef PLUENGINE_PHYSICSSPHERECOMPONENT_H
#define PLUENGINE_PHYSICSSPHERECOMPONENT_H
#include "PhysicsBodyComponent.h"
#include "PluEngine/Core.h"
#include "PhysicsSphereComponent.generated.h"

namespace Plu
{
	PLU_CLASS(PyExport)
	class PLUGAMEPLAY_API PhysicsSphereComponent : public PhysicsBodyComponent
	{
		REFLECTION_BODY_PHYSICSSPHERECOMPONENT()
	public:
		PhysicsSphereComponent();
		virtual ~PhysicsSphereComponent() override;

		PLU_PROPERTY(PyExport)
		float SphereRadius;

		JPH::ShapeRefC GetShape() override;
	};
}

#endif //PLUENGINE_PHYSICSBOXCOMPONENT_H