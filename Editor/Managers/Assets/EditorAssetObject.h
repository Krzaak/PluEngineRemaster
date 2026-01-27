//
// Created by Plutex on 10.01.2026.
//

#ifndef PLUENGINE_EDITORASSETOBJECT_H
#define PLUENGINE_EDITORASSETOBJECT_H
#include "PluEngine/Objects/EngineObject.h"
#include "Path/Path.h"
#include "PluEngine/Managers/AssetsManager.h"
#include "EditorAssetObject.generated.h"

namespace Plu
{
    PLU_CLASS(Abstract)
    class IEditorAssetObject : public EngineObject
    {
        REFLECTION_BODY_IEDITORASSETOBJECT()
    private:
        PathW mAssetPath = L"";
        String mAssetType;
        friend class EditorAssetManager;
    public:
        IEditorAssetObject() = default;
        virtual ~IEditorAssetObject() override = default;
        [[nodiscard]] String GetAssetName() const
        {
            return String::FromWide(mAssetPath.GetStem().CStr());
        }
        [[nodiscard]] PathW GetAssetPath() const
        {
            return mAssetPath;
        }

        virtual IAssetInfo* GetAssetInfoPtr() = 0;
    };

    template<typename T>
    class EditorAssetObject : public IEditorAssetObject
    {
    public:
        T AssetInfo;

        IAssetInfo *GetAssetInfoPtr() override
        {
            return static_cast<IAssetInfo *>(&AssetInfo);
        }
    };
}

#endif //PLUENGINE_EDITORASSETOBJECT_H
