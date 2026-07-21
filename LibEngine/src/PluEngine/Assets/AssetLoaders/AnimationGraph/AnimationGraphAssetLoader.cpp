//
// Created by Plutex on 7/20/26.
//

#include "PluEngine/Assets/AssetLoaders/AnimationGraph/AnimationGraphAssetLoader.h"

#include "PluEngine/AssetTypes/AnimationGraph/AnimationGraph.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Reflection/ReflectionBase.h"
#include "PluEngine/Reflection/TypeTraits.h"
#include "PluEngine/NodeGraph/NodeGraphSerializer.h"

Plu::String Plu::AnimationGraphAssetLoader::GetSupportedAssetType()
{
    return AnimationGraph::GetStaticClass()->TypeName;
}

bool Plu::AnimationGraphAssetLoader::LoadAssetData(TUsePointer<AssetDescriptor> assetDesc,
    TOwningPointer<IAssetData> *assetDataToPopulate, TUsePointer<EngineAssetManager> assetManager,
    TUsePointer<EngineObjectManager> objectManager, TUsePointer<SceneManager> sceneManager,
    TUsePointer<IShaderManager> shaderManager)
{
    // Registering a loader takes this type off EngineAssetManager's generic JSON path. The graph's
    // own reflected fields ride TypeSerializer<TypeInfo*>, but the polymorphic node list + links need
    // the hand-written NodeGraphSerializer (the generic array serializer drops subclass identity).
    std::optional<JSON> jsonOpt = DiskManager::LoadJson(assetDesc->AssetPath.ToString().ToWide());
    if (!jsonOpt.has_value()) {
        PLU_CORE_ERROR("Failed to read AnimationGraph JSON at {}", assetDesc->AssetPath.ToString().CStr());
        return false;
    }

    DeserializationContext dc;
    dc.assetManager  = assetManager;
    dc.scenesManager = sceneManager;
    dc.shaderManager = shaderManager;

    auto* graph = static_cast<AnimationGraph*>(AnimationGraph::GetStaticClass()->Construct());
    if (!graph) return false;
    // Reflected fields on the graph itself (Uuid, plus any future PLU_PROPERTY).
    TypeSerializer<TypeInfo*>::Deserialize(&dc, jsonOpt.value(), AnimationGraph::GetStaticClass(), graph);
    // Polymorphic nodes + links.
    NodeGraphSerializer::Load(&dc, *graph, jsonOpt.value());

    *assetDataToPopulate = TOwningPointer(static_cast<IAssetData*>(graph));
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
    // The graph owns a polymorphic node list, which the generic reflection serializer can't handle
    // (it would drop each node's concrete type). So we write the file ourselves: reflected fields via
    // TypeSerializer<TypeInfo*>, then nodes + links via NodeGraphSerializer. Returning true tells
    // EngineAssetManager we've persisted it, so it does not fall through to the generic path.
    TUsePointer<IAssetData> data = assetManager->GetAssetData(assetDesc);
    auto* graph = dynamic_cast<NodeGraph*>(data.GetRaw());
    if (!graph) {
        PLU_CORE_ERROR("AnimationGraph save: asset data is not a NodeGraph");
        return false;
    }

    JSON json = TypeSerializer<TypeInfo*>::Serialize(assetDesc->AssetType, data.GetRaw());
    NodeGraphSerializer::Save(*graph, json);
    json["uuid"] = graph->Uuid.getUUID();
    DiskManager::SaveJson(assetDesc->AssetPath.ToString().ToWide(), json);
    return true;
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
