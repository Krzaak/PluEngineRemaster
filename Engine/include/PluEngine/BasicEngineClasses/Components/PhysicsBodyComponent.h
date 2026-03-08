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
	class PhysicsBody;
	PLU_CLASS()
	class PLU_API PhysicsBodyComponent : public WorldComponent
	{
		REFLECTION_BODY_PHYSICSBODYCOMPONENT()
	private:
		TOwningPointer<PhysicsBody> mPhysicsBody;
	public:
		PhysicsBodyComponent();
		virtual ~PhysicsBodyComponent() override;

		PLU_PROPERTY()
		BodyType PhysicsBodyType;
	};
}

#endif //PLUENGINE_PHYSICSBODYCOMPONENT_H
