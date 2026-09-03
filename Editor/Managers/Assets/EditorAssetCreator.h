//
// Created by Plutex on 5/21/26.
//

#ifndef PLUENGINE_EDITORASSETCREATOR_H
#define PLUENGINE_EDITORASSETCREATOR_H
#include "PluEngine/Core/Objects/EngineObject.h"
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
        // Directory the new asset file lands in. Empty falls back to the project Assets root.
        PathW mTargetDirectory;

        bool mFirstTime = true;
    public:
        EditorAssetCreator() = default;
        virtual ~EditorAssetCreator() override = default;

        void Initialize(TypeInfo* assetClass, const TUsePointer<EngineAssetManager> &assetManager, const PathW& targetDirectory = PathW());
        void RenderUI();
    };
}



#endif //PLUENGINE_EDITORASSETCREATOR_H
