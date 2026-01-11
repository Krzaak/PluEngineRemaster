//
// Created by Plutex on 10.01.2026.
//

#ifndef PLUENGINE_EDITORASSETOBJECT_H
#define PLUENGINE_EDITORASSETOBJECT_H
#include "PluEngine/Objects/EngineObject.h"
#include "Path/Path.h"
#include "PluEngine/Managers/AssetsManager.h"
#include <IEditorAssetObject.generated.h>

namespace Plu
{
    PLU_CLASS(Abstract)
    class IEditorAssetObject : public EngineObject
    {
        REFLECTION_BODY_IEDITORASSETOBJECT()
    private:
        PathW mAssetPath;
    public:
        String GetAssetName() const
        {
            return String::FromWide(mAssetPath.GetStem().CStr());
        }
    };

    PLU_CLASS()
    template<typename T>
    class EditorAssetObject : public IEditorAssetObject
    {
    public:
        T AssetInfo;
    };
}

#endif //PLUENGINE_EDITORASSETOBJECT_H
