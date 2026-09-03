//
// Created by Plutex on 8/12/26.
//

#ifndef PLUENGINE_ASSETREFLECTIONHOOKS_H
#define PLUENGINE_ASSETREFLECTIONHOOKS_H
#include "PluEngine/Core.h"

namespace Plu
{
    // Teaches reflection how to reach the asset system. Reflection lives in PluCore, below the
    // asset layer, so it holds function objects rather than naming EngineAssetManager; this fills
    // them in. Call once during engine init, before anything deserializes an asset reference.
    PLUASSETCORE_API void InstallAssetReflectionHooks();
}

#endif //PLUENGINE_ASSETREFLECTIONHOOKS_H
