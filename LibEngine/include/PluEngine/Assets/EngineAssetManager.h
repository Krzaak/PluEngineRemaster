//
// Created by Plutex on 5/20/26.
//

#ifndef PLUENGINE_ENGINEASSETMANAGER_H
#define PLUENGINE_ENGINEASSETMANAGER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "EngineAssetManager.generated.h"
#include "PluEngine/PluUUID.h"
#include "PluEngine/Threading/ThreadAffinity.h"
#include "HashSet/HashSet.h"
#include <shared_mutex>
#include <mutex>

namespace Plu
{
    class IAssetLoader;
    struct IAssetData;
    struct AssetDescriptor;


    PLU_CLASS()
    class PLU_API EngineAssetManager : public EngineObject
    {
        REFLECTION_BODY_ENGINEASSETMANAGER()
    private:
        GameHashMap<UInt64, TOwningPointer<AssetDescriptor>> mAssetMap;
        GameHashMap<UInt64, Path> mAssetPathMap;

        GameHashMap<UInt64, TOwningPointer<IAssetData>> mAssetDataMap;
        GameHashMap<Path, UInt64> mAssetPathByUUIDMap;
#ifdef PLU_ENGINE_EDITOR_BUILD
        void DispatchAssetSaveBinary(PluUUID uuid);
        void DispatchAssetSaveJSON(PluUUID uuid);

        HashSet<UInt64> mDirtyAssets;
#endif

        void LoadJSONAssetData(TUsePointer<AssetDescriptor> assetDesc);
        void LoadBinaryAssetData(TUsePointer<AssetDescriptor> assetDesc);

        PluUUID LoadJSONDescriptor(const Path &assetPath);
        PluUUID LoadBinaryDescriptor(Path assetPath);

        ApplicationInfo* mApplicationInfo = nullptr;

        GameHashMap<String,TOwningPointer<IAssetLoader>> mAssetLoaders;
        void RegisterAssetDataFromLoader(TOwningPointer<IAssetData> assetData, TUsePointer<AssetDescriptor> assetDesc);
        friend class IAssetLoader;

        // Protects mAssetMap, mAssetPathMap, mAssetPathByUUIDMap, mAssetDataMap for
        // concurrent read access from the render thread. Writers (main-thread-only) take
        // unique_lock; readers (any thread) take shared_lock. mAssetLoaders is setup-only
        // (PrepareLoaders at startup) and never read by render thread — not protected.
        mutable std::shared_mutex mMutex;

        // Deferred load requests posted by off-main threads (e.g. the render thread when
        // GetAssetDataNoLoad misses). Drained on the main thread by ProcessPendingLoads().
        // HashSet dedupes repeated requests for the same UUID while it is still pending.
        HashSet<UInt64> mPendingLoadRequests;
        mutable std::mutex mPendingLoadMutex;

        // Mutations remain main-thread-only. Asserts in debug, no-op in release.
        void CheckOwnerThread() const
        {
            PLU_CORE_ASSERT(IsOnMainThread(), "EngineAssetManager mutation off the main thread");
        }
    public:
        EngineAssetManager();
        virtual ~EngineAssetManager() override;

        //Setup
        void Initialize(ApplicationInfo* appInfo);
        void PrepareLoaders();

        //Loading
        void LoadAssetDescriptor(Path assetPath);
        void ScanDirectory(const Path &assetPath, bool engineAssets = false);
        void LoadAssetData(TUsePointer<AssetDescriptor> assetDesc);

        //Getters (thread-safe — safe to call from any thread)
        TUsePointer<AssetDescriptor> GetAssetDescriptor(PluUUID uuid);
        TUsePointer<IAssetData> GetAssetData(PluUUID uuid);
        TUsePointer<IAssetData> GetAssetData(TUsePointer<AssetDescriptor> assetDesc);
        // Non-lazy-loading variant — returns nullptr if asset not yet loaded. Use from render thread.
        [[nodiscard]] TUsePointer<IAssetData> GetAssetDataNoLoad(PluUUID uuid) const;
        TUsePointer<IAssetLoader> GetAssetLoader(TypeInfo* type);

        // Deferred loading (thread-safe — call from any thread). Posts a request to load the
        // asset's CPU data; the actual I/O happens on the main thread in ProcessPendingLoads().
        // Use from the render thread when GetAssetDataNoLoad misses, then re-check next frame.
        void RequestAssetDataLoad(PluUUID uuid);
        // Drains the pending-load queue. Main-thread only — call once per frame.
        void ProcessPendingLoads();

        //Getters for Paths
        Path GetAssetPath(PluUUID uuid);
        Path GetAssetPath(TUsePointer<AssetDescriptor> assetDesc);

        //Validation
        [[nodiscard]] bool AssetExists(PluUUID uuid) const;
        [[nodiscard]] bool AssetExistsInPath(Path assetPath) const;
        [[nodiscard]] bool IsAssetLoaded(PluUUID uuid) const;

#ifdef PLU_ENGINE_EDITOR_BUILD
        //Getters
        TUsePointer<AssetDescriptor> GetAssetDescriptor(Path assetPath);
        TUsePointer<IAssetData> GetAssetData(Path assetPath);
        TUsePointer<IAssetLoader> GetAssetLoaderForExtension(String extension);

        //Validation
        bool AssetExistsWithName(String assetName);

        //Slow Section
        DynamicArray<TUsePointer<AssetDescriptor>> GetAllAssetDescriptorsOfType(TypeInfo* type);

        //Utils
        void PrepareAssetsForDistribution(Path dir);
        void ConstructPythonAssetDictionary(Path file);

        //Dirtieness, btw only way do clean the asset is to save it
        void MarkAssetDirty(TUsePointer<AssetDescriptor> assetDesc);
        bool IsAssetDirty(TUsePointer<AssetDescriptor> assetDesc) const;
        bool AreAnyAssetsDirty() const;

        //Saving
        void SaveAsset(TUsePointer<AssetDescriptor> assetDesc);
        void SaveAsset(PluUUID uuid);
        void SaveAsset(TUsePointer<IAssetData> assetDesc);
#endif
    };

    PLU_FUNCTION(PyName=GetAssetByUUID)
    PLU_API TUsePointer<IAssetData> GetAssetByUUID(UInt64 uuid);

    PLU_API TUsePointer<IAssetData> GetAssetUserAsRaw(IAssetData* assetInfo);
}

#endif //PLUENGINE_ENGINEASSETMANAGER_H
