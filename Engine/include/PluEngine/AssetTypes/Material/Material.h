//
// Created by Plutex on 25.01.2026.
//

#ifndef PLUENGINE_MATERIAL_H
#define PLUENGINE_MATERIAL_H

#include "PluEngine/Core.h"
#include "PluEngine/Managers/AssetsManager.h"
#include "Material.generated.h"

namespace Plu
{
    struct StaticMesh;

    PLU_STRUCT()
    struct PLU_API Material : IAssetInfo
    {
        REFLECTION_BODY_MATERIAL()
    public:
        PLU_PROPERTY()
        TUsePointer<StaticMesh> staticMesh;
    };
}

#endif //PLUENGINE_MATERIAL_H
