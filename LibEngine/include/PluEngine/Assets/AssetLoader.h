//
// Created by Plutex on 5/22/26.
//

#ifndef PLUENGINE_ASSETLOADER_H
#define PLUENGINE_ASSETLOADER_H
#include "PluEngine/Core.h"
#include "PluEngine/Objects/EngineObject.h"
#include "AssetLoader.generated.h"

namespace Plu
{
    struct IAssetData;
    struct AssetDescriptor;
    PLU_CLASS(Abstract)
    class PLU_API IAssetLoader : public EngineObject
    {
        REFLECTION_BODY_IASSETLOADER()
    protected:

    public:
        virtual String GetSupportedAssetType() = 0;

        //Gives all the possible managers, bool return for optional fail information.
        virtual bool LoadAssetData(
            TUsePointer<AssetDescriptor> assetDesc,
            TOwningPointer<IAssetData>* assetDataToPopulate,
            TUsePointer<EngineAssetManager> assetManager,
            TUsePointer<EngineObjectManager> objectManager,
            TUsePointer<IScenesManager> sceneManager,
            TUsePointer<IShaderManager> shaderManager
        ) = 0;

        void DispatchAssetLoad(
            TUsePointer<AssetDescriptor> assetDesc,
            TUsePointer<EngineAssetManager> assetManager,
            TUsePointer<EngineObjectManager> objectManager,
            TUsePointer<IScenesManager> sceneManager,
            TUsePointer<IShaderManager> shaderManager
        );

        virtual TypeInfo* GetAssetTypeViewportClass() = 0;
    };
}

#endif //PLUENGINE_ASSETLOADER_H
