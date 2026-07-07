//
// Created by Plutex on 7/7/26.
//

#include "PluEngine/Assets/AssetLoaders/Animation/AnimationAssetLoader.h"

#include "PluEngine/Assets/AssetLoaders/SkeletalMesh/SkeletalMeshImporter.h"
#include "PluEngine/AssetTypes/Animation/SkeletalAnimation.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/EngineAssetManager.h"

Plu::String Plu::AnimationAssetLoader::GetSupportedAssetType()
{
    return Animation::GetStaticClass()->TypeName;
}

bool Plu::AnimationAssetLoader::LoadAssetData(TUsePointer<AssetDescriptor> assetDesc,
    TOwningPointer<IAssetData> *assetDataToPopulate, TUsePointer<EngineAssetManager> assetManager,
    TUsePointer<EngineObjectManager> objectManager, TUsePointer<SceneManager> sceneManager,
    TUsePointer<IShaderManager> shaderManager)
{
    TOwningPointer<Animation> animation = CreateOwning<Animation>();
    if (!LoadAnimation(assetDesc->AssetPath, *animation, assetManager)) return false;
    *assetDataToPopulate = animation;
    return true;
}

#ifdef PLU_ENGINE_EDITOR_BUILD
Plu::TypeInfo * Plu::AnimationAssetLoader::GetAssetTypeViewportClass()
{
    // Editor-side class, resolved by name so LibEngine stays free of an Editor include.
    return TypeRegistry::GetInstance()->GetTypeOfName("AnimationViewport");
}

bool Plu::AnimationAssetLoader::DispatchAssetSave(TUsePointer<AssetDescriptor> assetDesc,
    TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager,
    TUsePointer<SceneManager> sceneManager, TUsePointer<IShaderManager> shaderManager)
{
    TUsePointer<Animation> animation = assetManager->GetAssetData(assetDesc);
    if (!animation) return false;
    return SaveAnimation(assetDesc->AssetPath, *animation);
}

bool Plu::AnimationAssetLoader::IsAssetCreatable()
{
    return false;
}

bool Plu::AnimationAssetLoader::CanImportAsset(Path assetPath, TUsePointer<EngineAssetManager> assetManager,
    TUsePointer<EngineObjectManager> objectManager)
{
    return false;
}
#endif
