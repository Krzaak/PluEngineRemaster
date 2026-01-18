//
// Created by Plutex on 1/18/26.
//

#ifndef PLUENGINE_WORLDCOMPONENT_H
#define PLUENGINE_WORLDCOMPONENT_H
#include "GameObjectComponent.h"
#include "WorldComponent.generated.h"

namespace Plu
{
	PLU_CLASS()
	class PLU_API WorldComponent : public GameObjectComponent
	{
		REFLECTION_BODY_WORLDCOMPONENT()
	public:
		WorldComponent() = default;
		virtual ~WorldComponent() override = default;
	};
}

#endif //PLUENGINE_WORLDCOMPONENT_H
