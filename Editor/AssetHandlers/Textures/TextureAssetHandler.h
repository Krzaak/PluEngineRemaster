//
// Created by Plutex on 2026-02-07.
//

#ifndef PLUENGINE_TEXTUREASSETHANDLER_H
#define PLUENGINE_TEXTUREASSETHANDLER_H
#include "PluEngine/Core.h"
#include "TextureAssetHandler.generated.h"
#include "Managers/Assets/EditorAssetImporter.h"
#include "PluEngine/Assets/AssetLoader.h"

namespace Plu
{
	struct IAssetData;
	struct AssetDescriptor;
	PLU_CLASS()
	class TextureAssetHandler final : public IAssetLoader
	{
		REFLECTION_BODY_TEXTUREASSETHANDLER()
	public:
		TextureAssetHandler() = default;
		~TextureAssetHandler() override = default;

		TypeInfo *GetAssetTypeViewportClass() override;
		DynamicArray<String> GetSupportedImportExtensions() override;
		String GetSupportedAssetType() override;
		bool LoadAssetData(TUsePointer<AssetDescriptor> assetDesc, TOwningPointer<IAssetData> *assetDataToPopulate, TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager, TUsePointer<IScenesManager> sceneManager, TUsePointer<IShaderManager> shaderManager) override;
	};
}

#endif //PLUENGINE_TEXTUREASSETHANDLER_H