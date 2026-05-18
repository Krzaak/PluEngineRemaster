//
// Created by Plutex on 5/17/26.
//

#ifndef PLUENGINE_ASSETINFO_H
#define PLUENGINE_ASSETINFO_H

#include "PluEngine/Core.h"
#include "AssetInfo.generated.h"
#include "PluEngine/PluUUID.h"

namespace Plu
{
    struct IAssetData;

    PLU_STRUCT()
    struct PLU_API AssetInfo
    {
        REFLECTION_BODY_ASSETINFO()

        PluUUID Uuid;
        TypeInfo* AssetClass;
        TUsePointer<IAssetData> AssetData;

#ifdef PLU_EDITOR
        String AssetPath;
        String GetAssetName();
#endif

    };
}

#endif //PLUENGINE_ASSETINFO_H
