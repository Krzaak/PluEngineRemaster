//
// Created by Plutex on 8/12/26.
//

#include "PluEngine/Core/Reflection/TypeTraits.h"

// Not inside PLU_ENGINE_EDITOR_BUILD: loading a scene resolves asset UUIDs in every build.
Plu::TUsePointer<Plu::IAssetData> Plu::ResolveAssetData(DeserializationContext* dc, const PluUUID& uuid)
{
    const auto& resolver = TypeRegistry::GetInstance()->assetDataResolver;
    if (!resolver) {
        PLU_CORE_ERROR("ResolveAssetData: asset layer not installed, asset {} stays null", uuid.getUUID());
        return nullptr;
    }
    return resolver(dc, uuid);
}
