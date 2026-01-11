//
// Created by Plutex on 1/11/26.
//

#ifndef PLUENGINE_GAMEOBJECT_H
#define PLUENGINE_GAMEOBJECT_H

#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "GameObject.generated.h"

namespace Plu
{
	PLU_CLASS(Abstract)
	class PLU_API GameObject : public EngineObject
	{
		REFLECTION_BODY_GAMEOBJECT()
	public:
		//TODO
	};
}

#endif //PLUENGINE_GAMEOBJECT_H
