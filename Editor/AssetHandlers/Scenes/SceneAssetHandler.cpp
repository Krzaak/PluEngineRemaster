//
// Created by Plutex on 1/12/26.
//

#include "SceneAssetHandler.h"

#include "json_fwd.hpp"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Assets/EditorAssetObject.h"
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

bool Plu::SceneAssetHandler::LoadAsset(PathW path, TUsePointer<EditorProjectManager> editorProjectManager,
	TUsePointer<EngineObjectManager> engineObjectManager, EditorAssetManager *editorAssetManager)
{
	EngineObjectHandle assetObject = engineObjectManager->CreateObject<EditorAssetObject<SceneInfo>>();
	TOwningPointer<EditorAssetObject<SceneInfo>> assetObjectT = engineObjectManager->GetObjectAsOwner<EditorAssetObject<SceneInfo>>(assetObject);
	std::optional<nlohmann::json> jsonOpt = DiskManager::LoadJson(path);
	if (!jsonOpt.has_value()) return false;
	const nlohmann::json& json = jsonOpt.value();
	if (!json.contains("type")) {
		PLU_ERROR("Asset at: {} is invalid JSON format");
		return false;
	}
	editorAssetManager->AddAssetFromHandler(assetObjectT, editorMeshData.uuid);
}
