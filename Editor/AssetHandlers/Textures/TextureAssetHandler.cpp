//
// Created by Plutex on 2026-02-07.
//

#include "TextureAssetHandler.h"
#include "TextureImporter.h"
#include "DefinedViewports/Texture/TextureViewport.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"
#include "PluEngine/Objects/EngineObjectManager.h"

Plu::TypeInfo * Plu::TextureAssetHandler::GetAssetTypeViewportClass()
{
	return TextureViewport::GetStaticClass();
}

DynamicArray<Plu::String> Plu::TextureAssetHandler::GetSupportedImportExtensions()
{
	return  {".png"};
}

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
