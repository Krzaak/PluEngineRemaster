//
// Created by Plutex on 1/18/26.
//

#ifndef PLUENGINE_WORLDCOMPONENT_H
#define PLUENGINE_WORLDCOMPONENT_H
#include "GameObjectComponent.h"
#include "WorldComponent.generated.h"
#include "PluEngine/PluTypes.h"

namespace Plu
{
	PLU_CLASS()
	class PLU_API WorldComponent : public GameObjectComponent
	{
		REFLECTION_BODY_WORLDCOMPONENT()
	private:
	public:
		WorldComponent() = default;
		virtual ~WorldComponent() override = default;

		Vec3 GetWorldLocation();
		Vec3 GetWorldRotation();
		Vec3 GetWorldScale();
	};
}

#endif //PLUENGINE_WORLDCOMPONENT_H
