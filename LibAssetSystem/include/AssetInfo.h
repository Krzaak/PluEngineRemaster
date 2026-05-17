//
// Created by Plutex on 5/17/26.
//

#ifndef PLUENGINE_ASSETINFO_H
#define PLUENGINE_ASSETINFO_H

#include "PluEngine/Core.h"
#include "AssetInfo.generated.h"
#include "PluEngine/Objects/EngineObject.h"

namespace Plu
{
    PLU_STRUCT()
    struct PLU_API AssetInfo
    {
        REFLECTION_BODY_ASSETINFO()

#ifdef PLU_EDITOR
        dsad
#endif

    };
}

#endif //PLUENGINE_ASSETINFO_H
