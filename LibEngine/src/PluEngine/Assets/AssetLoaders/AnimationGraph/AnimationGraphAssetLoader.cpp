//
// Created by Plutex on 7/20/26.
//

#include "PluEngine/Assets/AssetLoaders/AnimationGraph/AnimationGraphAssetLoader.h"

#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Reflection/ReflectionBase.h"

Plu::String Plu::AnimationGraphAssetLoader::GetSupportedAssetType()
{
    return AnimationGraph::GetStaticClass()->TypeName;
}

bool Plu::AnimationGraphAssetLoader::LoadAssetData(TUsePointer<AssetDescriptor> assetDesc,
    TOwningPointer<IAssetData> *assetDataToPopulate, TUsePointer<EngineAssetManager> assetManager,
    TUsePointer<EngineObjectManager> objectManager, TUsePointer<SceneManager> sceneManager,
    TUsePointer<IShaderManager> shaderManager)
{
    // Registering a loader for a type takes it off EngineAssetManager's generic JSON path, so the
    // reflection-driven deserialize has to happen here. Every PLU_PROPERTY on AnimationGraph is
    // picked up automatically — this stays as-is however many the graph grows.
    std::optional<JSON> jsonOpt = DiskManager::LoadJson(assetDesc->AssetPath.ToString().ToWide());
    if (!jsonOpt.has_value()) {
        PLU_CORE_ERROR("Failed to read AnimationGraph JSON at {}", assetDesc->AssetPath.ToString().CStr());
        return false;
    }

    DeserializationContext dc;
    dc.assetManager  = assetManager;
    dc.scenesManager = sceneManager;
    dc.shaderManager = shaderManager;

    void* loaded = AnimationGraph::GetStaticClass()->DeSerializeFromJSON(&dc, jsonOpt.value());
    if (!loaded) return false;

    *assetDataToPopulate = TOwningPointer(static_cast<IAssetData*>(loaded));
    return true;
}

#ifdef PLU_ENGINE_EDITOR_BUILD
Plu::TypeInfo * Plu::AnimationGraphAssetLoader::GetAssetTypeViewportClass()
{
    // Editor-side class, resolved by name so LibEngine stays free of an Editor include.
    return TypeRegistry::GetInstance()->GetTypeOfName("AnimationGraphViewport");
}

bool Plu::AnimationGraphAssetLoader::DispatchAssetSave(TUsePointer<AssetDescriptor> assetDesc,
    TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager,
    TUsePointer<SceneManager> sceneManager, TUsePointer<IShaderManager> shaderManager)
{
    // false on purpose: EngineAssetManager::DispatchAssetSaveJSON falls through to the generic
    // reflection serializer, which already writes every PLU_PROPERTY plus the uuid. Only override
    // this once the graph needs a hand-written format.
    return false;
}

bool Plu::AnimationGraphAssetLoader::IsAssetCreatable()
{
    // Shows up under "Create Asset" in the asset browser.
    return true;
}

bool Plu::AnimationGraphAssetLoader::CanImportAsset(Path assetPath, TUsePointer<EngineAssetManager> assetManager,
    TUsePointer<EngineObjectManager> objectManager)
{
    // Authored in the editor, never imported from a source file.
    return false;
}
#endif
