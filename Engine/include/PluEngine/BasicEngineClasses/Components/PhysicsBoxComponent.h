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
	PLU_CLASS()
	class PLU_API PhysicsBoxComponent : public PhysicsBodyComponent
	{
		REFLECTION_BODY_PHYSICSBOXCOMPONENT()
	public:
		PhysicsBoxComponent();
		virtual ~PhysicsBoxComponent() override;

		PLU_PROPERTY()
		Vec3 BoxSize;

		void CreatePhysicsBody() override;
	};
}

#endif //PLUENGINE_PHYSICSBOXCOMPONENT_H