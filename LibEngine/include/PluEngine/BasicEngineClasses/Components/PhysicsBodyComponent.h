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

		PLU_PROPERTY()
		bool ActiveBody = false;
	};
}

#endif //PLUENGINE_PHYSICSBODYCOMPONENT_H
