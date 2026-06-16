//
// Created by Plutex on 5/21/26.
//

#ifndef PLUENGINE_EDITORASSETCREATOR_H
#define PLUENGINE_EDITORASSETCREATOR_H
#include "PluEngine/Objects/EngineObject.h"
#include "EditorAssetCreator.generated.h"

namespace Plu
{
    PLU_CLASS()
    class EditorAssetCreator : public EngineObject
    {
        REFLECTION_BODY_EDITORASSETCREATOR()
    private:
        TUsePointer<EngineAssetManager> mAssetManager;
        TypeInfo* mTypeInfo = nullptr;

        bool mFirstTime = true;
    public:
        EditorAssetCreator() = default;
        virtual ~EditorAssetCreator() override = default;

        void Initialize(TypeInfo* assetClass, const TUsePointer<EngineAssetManager> &assetManager);
        void RenderUI();
    };
}



#endif //PLUENGINE_EDITORASSETCREATOR_H
