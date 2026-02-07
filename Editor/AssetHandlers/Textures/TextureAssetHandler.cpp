//
// Created by Plutex on 2026-02-07.
//

#include "TextureAssetHandler.h"
#include "TextureImporter.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/AssetTypes/Texture/Texture.h"

Plu::TypeInfo * Plu::TextureAssetHandler::GetAssetViewportClass()
{
	return nullptr;
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
	return nullptr;
}
