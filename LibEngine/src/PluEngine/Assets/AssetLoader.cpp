//
// Created by Plutex on 5/22/26.
//

#include "PluEngine/Assets/AssetLoader.h"

#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/Managers/AssetsManager.h"

void Plu::IAssetLoader::DispatchAssetLoad(TUsePointer<AssetDescriptor> assetDesc,
                                          TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager,
                                          TUsePointer<IScenesManager> sceneManager, TUsePointer<IShaderManager> shaderManager)
{
    TOwningPointer<IAssetData> assetData;
    bool fail = !LoadAssetData(assetDesc, &assetData,assetManager, objectManager, sceneManager, shaderManager);
    if (!fail) {
        assetManager->RegisterAssetDataFromLoader(assetData, assetDesc);
    }
}
