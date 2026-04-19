//
// Created by Plutex on 4/19/26.
//

#ifndef PLUENGINE_LIGHTBASEOBJECT_H
#define PLUENGINE_LIGHTBASEOBJECT_H
#include "PluEngine/Core.h"
#include "PluEngine/GameObject/GameObject.h"
#include "LightBaseObject.generated.h"

namespace Plu
{
    PLU_CLASS(Abstract)
    class PLU_API LightBaseObject : public GameObject
    {
        REFLECTION_BODY_LIGHTBASEOBJECT()
    protected:
        Vec3 mLightColor;
        float mLightIntensity;
    };
}

#endif //PLUENGINE_LIGHTBASEOBJECT_H
