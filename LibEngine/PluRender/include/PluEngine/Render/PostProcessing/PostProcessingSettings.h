//
// Created by Plutex on 8/12/26.
//

#ifndef PLUENGINE_POSTPROCESSINGSETTINGS_H
#define PLUENGINE_POSTPROCESSINGSETTINGS_H

#include "PluEngine/Core.h"
#include "PostProcessingSettings.generated.h"

namespace Plu
{
    PLU_STRUCT()
    struct PLURENDER_API PostProcessingSettings
    {
        REFLECTION_BODY_POSTPROCESSINGSETTINGS()

    public:
        PLU_PROPERTY()
        float Saturation = 1.0f;
    };
}

#endif //PLUENGINE_POSTPROCESSINGSETTINGS_H
