//
// Created by Plutex on 1/12/26.
//

#include "SceneAssetHandler.h"

#include "json_fwd.hpp"
#include "DefinedViewports/Scene/SceneViewport.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Scene/EditorScene.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectHandle.h"
#include "PluEngine/Objects/EngineObjectManager.h"

DynamicArray<Plu::String> & Plu::SceneAssetHandler::GetImportableExtensions()
{
	DynamicArray<String> ohio;
	return ohio;
}

Plu::String Plu::SceneAssetHandler::GetSupportedAssetType()
{
	return {"Scene"};
}

bool Plu::SceneAssetHandler::ImportAsset(PathW origin, PathW loadTo)
{
	return true;
}

Plu::TUsePointer<Plu::IEditorAssetObject> Plu::SceneAssetHandler::LoadAsset(
	PathW path, TUsePointer<EditorProjectManager> editorProjectManager,
	TUsePointer<EngineObjectManager> engineObjectManager, EditorAssetManager *editorAssetManager)
{
	EngineObjectHandle assetObject = engineObjectManager->CreateObject<EditorAssetObject<SceneInfo>>();
	TOwningPointer<EditorAssetObject<SceneInfo>> assetObjectT = engineObjectManager->GetObjectAsOwner<EditorAssetObject<SceneInfo>>(assetObject);
	std::optional<nlohmann::json> jsonOpt = DiskManager::LoadJson(path);
	if (!jsonOpt.has_value()) return nullptr;
	const nlohmann::json& json = jsonOpt.value();
	if (!json.contains("type")) {
		PLU_ERROR("Asset at: {} is invalid JSON format");
		return nullptr;
	}
	assetObjectT->AssetInfo.URL = path.GetStem().ToNarrow();
	editorAssetManager->AddAssetFromHandler(assetObjectT, assetObjectT->AssetInfo.Uuid, path);
	return assetObjectT;
}

Plu::TypeInfo * Plu::SceneAssetHandler::GetAssetViewportClass()
{
	return SceneViewport::GetStaticClass();
}
