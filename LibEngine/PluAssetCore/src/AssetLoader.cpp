//
// Created by Plutex on 5/22/26.
//

#include "PluEngine/AssetCore/AssetLoader.h"

#include "PluEngine/AssetCore/EngineAssetManager.h"
#include "PluEngine/Core/IAssetData.h"

void Plu::IAssetLoader::DispatchAssetLoad(TUsePointer<AssetDescriptor> assetDesc,
                                          TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager,
                                          TUsePointer<SceneManager> sceneManager, TUsePointer<IShaderManager> shaderManager)
{
    TOwningPointer<IAssetData> assetData;
    bool fail = !LoadAssetData(assetDesc, &assetData,assetManager, objectManager, sceneManager, shaderManager);
    if (!fail) {
        assetManager->RegisterAssetDataFromLoader(assetData, assetDesc);
    }
}
