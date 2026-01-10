//
// Created by Plutex on 10.01.2026.
//

#ifndef PLUENGINE_EDITORASSETOBJECT_H
#define PLUENGINE_EDITORASSETOBJECT_H
#include "PluEngine/Objects/EngineObject.h"
#include "EditorAssetObject.generated.h"
#include "PluEngine/Managers/AssetsManager.h"

namespace Plu
{
    PLU_CLASS(Abstract)
    class EditorAssetObject : public EngineObject
    {
        REFLECTION_BODY_EDITORASSETOBJECT()
    private:
        String mAssetPath;
    public:
        IAssetInfo* mAsset;

    };
}

#endif //PLUENGINE_EDITORASSETOBJECT_H
