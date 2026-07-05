//
// Created by Plutex on 5/25/26.
//

#ifndef PLUENGINE_EDITORASSETIMPORTER_H
#define PLUENGINE_EDITORASSETIMPORTER_H
#include "PluEngine/Objects/EngineObject.h"
#include "EditorAssetImporter.generated.h"

namespace Plu
{
    class IAssetLoader;
    PLU_CLASS()
    class EditorAssetImporter : public EngineObject
    {
        REFLECTION_BODY_EDITORASSETIMPORTER()
    private:
        ApplicationInfo* mApplicationInfo = nullptr;
        bool mFirstTime = true;

        //Multi type asset handling
        GameHashMap<String, DynamicArray<Path>> mAssetPathsPerType;
        GameHashMap<String, TypeInfo*> mAssetImportSettingsPerType;
        GameHashMap<String, void*> mAssetImportSettingsPerTypeData;
        GameHashMap<String, TUsePointer<IAssetLoader>> mAssetLoaderPerType;
    public:
        EditorAssetImporter() = default;
        virtual ~EditorAssetImporter() override = default;

        void Initialize(DynamicArray<Path> assetPaths, ApplicationInfo* appInfo);
        void RenderUI();
    };
}



#endif //PLUENGINE_EDITORASSETIMPORTER_H
