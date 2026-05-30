//
// Created by Plutex on 1/12/26.
//

#ifndef PLUENGINE_SCENEASSETHANDLER_H
#define PLUENGINE_SCENEASSETHANDLER_H
#include "PluEngine/Core.h"
#include "SceneAssetHandler.generated.h"
#include "Managers/Assets/EditorAssetImporter.h"
#include "PluEngine/Assets/AssetLoader.h"

namespace Plu
{
	PLU_CLASS()
	class SceneAssetHandler : public IAssetLoader
	{
		REFLECTION_BODY_SCENEASSETHANDLER()
	public:
		SceneAssetHandler() = default;
		~SceneAssetHandler() override = default;

		DynamicArray<String> &GetImportableExtensions();
		String GetSupportedAssetType() override;
		bool ImportAsset(PathW origin, PathW loadTo);
		bool LoadAssetData(TUsePointer<AssetDescriptor> assetDesc, TOwningPointer<IAssetData> *assetDataToPopulate,
		                   TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager,
		                   TUsePointer<SceneManager> sceneManager,
		                   TUsePointer<IShaderManager> shaderManager) override;
		TypeInfo *GetAssetTypeViewportClass() override;
	};
}

#endif //PLUENGINE_SCENEASSETHANDLER_H