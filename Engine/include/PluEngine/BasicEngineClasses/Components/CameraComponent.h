//
// Created by Plutex on 1/19/26.
//

#ifndef PLUENGINE_CAMERACOMPONENT_H
#define PLUENGINE_CAMERACOMPONENT_H
#include "PluEngine/GameObject/WorldComponent.h"
#include "CameraComponent.generated.h"

namespace Plu
{
	PLU_CLASS()
	class PLU_API CameraComponent : public WorldComponent
	{
		REFLECTION_BODY_CAMERACOMPONENT()
	};
}

#endif //PLUENGINE_CAMERACOMPONENT_H