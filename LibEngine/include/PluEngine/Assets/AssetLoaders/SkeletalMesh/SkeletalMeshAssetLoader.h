//
// Created by Plutex on 7/5/26.
//

#ifndef PLUENGINE_SKELETALMESHASSETLOADER_H
#define PLUENGINE_SKELETALMESHASSETLOADER_H
#include "PluEngine/Core.h"
#include "PluEngine/Assets/AssetLoader.h"
#include "SkeletalMeshAssetLoader.generated.h"

namespace Plu
{

    PLU_STRUCT()
    struct SkeletalMeshImportOptions
    {
        REFLECTION_BODY_SKELETALMESHIMPORTOPTIONS()

        PLU_PROPERTY()
        float Scale = 1.0f;

        PLU_PROPERTY()
        bool FlipUVs = true;

        PLU_PROPERTY()
        bool GenerateNormals = false;
    };

    PLU_CLASS()
    class PLU_API SkeletalMeshAssetLoader : public IAssetLoader
    {
        REFLECTION_BODY_SKELETALMESHASSETLOADER()
    public:
        SkeletalMeshAssetLoader() = default;
        virtual ~SkeletalMeshAssetLoader() override = default;

        String GetSupportedAssetType() override;
        bool LoadAssetData(TUsePointer<AssetDescriptor> assetDesc, TOwningPointer<IAssetData> *assetDataToPopulate,
                           TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager,
                           TUsePointer<SceneManager> sceneManager,
                           TUsePointer<IShaderManager> shaderManager) override;

#ifdef PLU_ENGINE_EDITOR_BUILD
        DynamicArray<String> GetSupportedImportExtensions() override;
        TypeInfo *GetImportSettingsClass() override;
        void HandleAssetImporting(DynamicArray<Path> &assetPaths, Path outPath, void *importSettings, TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager) override;
        TypeInfo *GetAssetTypeViewportClass() override;
        bool DispatchAssetSave(TUsePointer<AssetDescriptor> assetDesc, TUsePointer<EngineAssetManager> assetManager,
                               TUsePointer<EngineObjectManager> objectManager, TUsePointer<SceneManager> sceneManager,
                               TUsePointer<IShaderManager> shaderManager) override;
        bool IsAssetCreatable() override;
        bool CanImportAsset(Path assetPath, TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager) override;
#endif
    };
}

#endif //PLUENGINE_SKELETALMESHASSETLOADER_H
