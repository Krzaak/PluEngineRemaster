//
// Created by Plutex on 4/19/26.
//

#ifndef PLUENGINE_LIGHTBASEOBJECT_H
#define PLUENGINE_LIGHTBASEOBJECT_H
#include "PluEngine/Core.h"
#include "PluEngine/Gameplay/GameObject.h"
#include "LightBaseObject.generated.h"

namespace Plu
{
    PLU_CLASS(Abstract, PyExport)
    class PLUGAMEPLAY_API LightBaseObject : public GameObject
    {
        REFLECTION_BODY_LIGHTBASEOBJECT()
    protected:
        PLU_PROPERTY()
        Vec3 mLightColor = Vec3(1.0f, 1.0f, 1.0f);
        PLU_PROPERTY()
        float mLightIntensity = 1.0f;
    public:
        PLU_FUNCTION(PyExport)
        [[nodiscard]] Vec3 GetLightColor() const {return mLightColor;}
        PLU_FUNCTION(PyExport)
        [[nodiscard]] float GetLightIntensity() const {return mLightIntensity;}
    };
}

#endif //PLUENGINE_LIGHTBASEOBJECT_H
