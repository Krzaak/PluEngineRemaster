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
	PLU_CLASS(Abstract)
	class PLU_API PhysicsBodyComponent : public WorldComponent
	{
		REFLECTION_BODY_PHYSICSBODYCOMPONENT()
	protected:
		TOwningPointer<PhysicsBody> mPhysicsBody;
	public:
		PhysicsBodyComponent();
		virtual ~PhysicsBodyComponent() override;

		virtual void CreatePhysicsBody() = 0;
		void SyncParentFromPhysics();

		PLU_PROPERTY()
		BodyType PhysicsBodyType;
	};
}

#endif //PLUENGINE_PHYSICSBODYCOMPONENT_H
