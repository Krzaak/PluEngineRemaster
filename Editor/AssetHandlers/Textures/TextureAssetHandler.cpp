//
// Created by Plutex on 2026-02-07.
//

#include "TextureAssetHandler.h"
#include "TextureImporter.h"
#include "DefinedViewports/Texture/TextureViewport.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/AssetTypes/StaticMesh/StaticMesh.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"
#include "PluEngine/Objects/EngineObjectManager.h"

Plu::TypeInfo * Plu::TextureAssetHandler::GetAssetViewportClass()
{
	return TextureViewport::GetStaticClass();
}

DynamicArray<Plu::String> & Plu::TextureAssetHandler::GetImportableExtensions()
{
	static DynamicArray<String> extensions = {".png"};
	return extensions;
}

Plu::String Plu::TextureAssetHandler::GetSupportedAssetType()
{
	return "TextureInfo";
}

bool Plu::TextureAssetHandler::ImportAsset(PathW origin, PathW loadTo)
{
	PLU_INFO("Importing: {} into: {}", origin.ToString().ToNarrow().CStr(), loadTo.ToString().ToNarrow().CStr());
	PathW savePath = loadTo;
	savePath /= origin.GetStem() + PLU_BINARY_EXT_W;
	return TextureImport::ImportTexture(origin, savePath);
}

Plu::TUsePointer<Plu::IEditorAssetObject> Plu::TextureAssetHandler::LoadAsset(PathW path,
	TUsePointer<EditorProjectManager> editorProjectManager, TUsePointer<EngineObjectManager> engineObjectManager,
	EditorAssetManager *editorAssetManager)
{
	EngineObjectHandle assetObject = engineObjectManager->CreateObject<EditorAssetObject<TextureInfo>>();
	TOwningPointer<IEditorAssetObject> assetObjectTI = engineObjectManager->GetObjectAsOwner<IEditorAssetObject>(assetObject);
	TOwningPointer<EditorAssetObject<TextureInfo>> assetObjectT = DynamicCast<EditorAssetObject<TextureInfo>>(assetObjectTI);
	TextureImport::LoadTexture(path, assetObjectT->AssetInfo.GetRaw());
	editorAssetManager->AddAssetFromHandler(assetObjectT, assetObjectT->AssetInfo->Uuid, path, TextureInfo::GetStaticClass());
	return assetObjectT;
}
