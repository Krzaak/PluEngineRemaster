//
// Created by Plutex on 4/19/26.
//

#ifndef PLUENGINE_DIRECTIONALLIGHT_H
#define PLUENGINE_DIRECTIONALLIGHT_H
#include "LightBaseObject.h"
#include "PluEngine/Core.h"
#include "DirectionalLight.generated.h"

namespace Plu
{
    PLU_CLASS()
    class PLU_API DirectionalLight : public LightBaseObject
    {
        REFLECTION_BODY_DIRECTIONALLIGHT()
    public:
        DirectionalLight() = default;
        virtual ~DirectionalLight() override = default;
    };
}

#endif //PLUENGINE_DIRECTIONALLIGHT_H
