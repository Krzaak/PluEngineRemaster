//
// Created by Plutex on 5/20/26.
//

#ifndef PLUENGINE_ENGINEASSETMANAGER_H
#define PLUENGINE_ENGINEASSETMANAGER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "EngineAssetManager.generated.h"
#include "PluEngine/PluUUID.h"

namespace Plu
{
    struct IAssetData;
    struct AssetDescriptor;


    PLU_CLASS()
    class PLU_API EngineAssetManager : public EngineObject
    {
        REFLECTION_BODY_ENGINEASSETMANAGER()
    private:
        GameHashMap<UInt64, TOwningPointer<AssetDescriptor>> mAssetMap;
        GameHashMap<UInt64, Path> mAssetPathMap;

        GameHashMap<UInt64, TUsePointer<IAssetData>> mAssetDataMap;
#ifdef PLU_ENGINE_EDITOR_BUILD
        GameHashMap<Path, UInt64> mAssetPathByUUIDMap;

        void DispatchAssetSaveBinary(PluUUID uuid);
        void DispatchAssetSaveJSON(PluUUID uuid);
#endif

        void LoadJSONDescriptor(const Path &assetPath);
        void LoadBinaryDescriptor(Path assetPath);
    public:
        EngineAssetManager();
        virtual ~EngineAssetManager() override;

        //Loading
        void LoadAssetDescriptor(Path assetPath);
        void ScanDirectory(const Path &assetPath);
        void LoadAssetData(TUsePointer<AssetDescriptor> assetDesc);

        //Getters
        TUsePointer<AssetDescriptor> GetAssetDescriptor(PluUUID uuid);
        TUsePointer<IAssetData> GetAssetData(PluUUID uuid);
        TUsePointer<IAssetData> GetAssetData(TUsePointer<AssetDescriptor> assetDesc);

        //Validation
        [[nodiscard]] bool AssetExists(PluUUID uuid) const;

#ifdef PLU_ENGINE_EDITOR_BUILD
        //Getters
        TUsePointer<AssetDescriptor> GetAssetDescriptor(Path assetPath);
        TUsePointer<IAssetData> GetAssetData(Path assetPath);

        //Getters for Paths
        Path GetAssetPath(PluUUID uuid);
        Path GetAssetPath(TUsePointer<AssetDescriptor> assetDesc);

        //Validation
        [[nodiscard]] bool AssetExistsInPath(Path assetPath) const;
        bool AssetExistsWithName(String assetName);

        //Slow Section
        DynamicArray<TUsePointer<AssetDescriptor>> GetAllAssetDescriptorsOfType(TypeInfo* type);

        //Utils
        void ImportAssets(DynamicArray<Path> assetPaths, Path importTo);

        //Saving
        void SaveAsset(TUsePointer<AssetDescriptor> assetDesc);
        void SaveAsset(PluUUID uuid);
        void SaveAsset(TUsePointer<IAssetData> assetDesc);
#endif
    };
}

#endif //PLUENGINE_ENGINEASSETMANAGER_H
