//
// Created by Plutex on 5/30/26.
//

#ifndef PLUENGINE_PLUGAME_H
#define PLUENGINE_PLUGAME_H

#include "PluGame.generated.h"
#include "Core.h"
#include "PluUUID.h"

namespace Plu
{
    struct SceneInfo;
    PLU_STRUCT()
    struct PLUCORE_API GameStartupSettings
    {
        REFLECTION_BODY_GAMESTARTUPSETTINGS()
    public:
        PLU_PROPERTY()
        TUsePointer<SceneInfo> GameStartupScene;

        PLU_PROPERTY()
        TUsePointer<SceneInfo> EditorStartupScene;
    };
}

#endif //PLUENGINE_PLUGAME_H
