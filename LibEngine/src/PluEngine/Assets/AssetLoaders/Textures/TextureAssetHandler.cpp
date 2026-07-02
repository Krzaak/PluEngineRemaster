//
// Created by Plutex on 2026-02-07.
//

#include "PluEngine/Assets/AssetLoaders/Textures/TextureAssetHandler.h"
#include "PluEngine/Assets/AssetLoaders/Textures/TextureImporter.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Assets/EngineAssetManager.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"
#include "PluEngine/Objects/EngineObjectManager.h"

#ifdef PLU_ENGINE_EDITOR_BUILD

Plu::TypeInfo * Plu::TextureAssetHandler::GetAssetTypeViewportClass()
{
	return TypeRegistry::GetInstance()->GetTypeOfName("TextureViewport");
}

DynamicArray<Plu::String> Plu::TextureAssetHandler::GetSupportedImportExtensions()
{
	return  {".png"};
}

Plu::TypeInfo * Plu::TextureAssetHandler::GetImportSettingsClass()
{
	return nullptr;
}

void Plu::TextureAssetHandler::HandleAssetImporting(DynamicArray<Path> &assetPaths, Path outPath, void *importSettings,
	TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager)
{
	for (auto path : assetPaths) {
		PathW outPathForAsset = outPath.ToString().ToWide();
		outPathForAsset += String::Format("/{0}{1}", path.GetStem().CStr(), PLU_BINARY_EXT).ToWide();
		TextureImport::ImportTexture(path.ToString().ToWide(), outPathForAsset);
		assetManager->LoadAssetDescriptor(outPathForAsset.ToString().ToNarrow());
	}
}

bool Plu::TextureAssetHandler::IsAssetCreatable()
{
	return false;
}

#endif

Plu::String Plu::TextureAssetHandler::GetSupportedAssetType()
{
	return "TextureInfo";
}

bool Plu::TextureAssetHandler::LoadAssetData(TUsePointer<AssetDescriptor> assetDesc,
	TOwningPointer<IAssetData> *assetDataToPopulate, TUsePointer<EngineAssetManager> assetManager,
	TUsePointer<EngineObjectManager> objectManager, TUsePointer<SceneManager> sceneManager,
	TUsePointer<IShaderManager> shaderManager)
{
	TOwningPointer<TextureInfo> textureInfo = CreateOwning<TextureInfo>();
	TextureImport::LoadTexture(assetDesc->AssetPath.ToString().ToWide(), textureInfo.GetRaw());
	*assetDataToPopulate = textureInfo;
	return true;
}
