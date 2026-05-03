//
// Created by Plutex on 5/3/26.
//

#ifndef PLUENGINE_EDITORSETTINGS_H
#define PLUENGINE_EDITORSETTINGS_H

#include "PluEngine/Core.h"
#include "EditorSettings.generated.h"

namespace Plu
{
    PLU_STRUCT()
    struct EditorSettings
    {
        REFLECTION_BODY_EDITORSETTINGS()
    public:
        PLU_PROPERTY()
        float EditorCameraScrollWheelSpeedMultiplier = 1.0f;
        PLU_PROPERTY()
        float EditorCameraPanSpeedMultiplier = 1.0f;
        PLU_PROPERTY()
        float EditorCameraMoveSpeedMultiplier = 1.0f;
        PLU_PROPERTY()
        float EditorCameraLookSpeedMultiplier = 1.0f;
    };
}

#endif //PLUENGINE_EDITORSETTINGS_H
