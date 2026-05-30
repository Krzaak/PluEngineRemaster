//
// Created by Plutex on 1/12/26.
//

#include "SceneAssetHandler.h"

#include "EditorAppContext.h"
#include "json_fwd.hpp"
#include "DefinedViewports/Scene/SceneViewport.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/Assets/AssetDescriptor.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectHandle.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Scenes/SceneManager.h"

DynamicArray<Plu::String> & Plu::SceneAssetHandler::GetImportableExtensions()
{
	static DynamicArray<String> ohio;
	return ohio;
}

Plu::String Plu::SceneAssetHandler::GetSupportedAssetType()
{
	return "SceneInfo";
}

bool Plu::SceneAssetHandler::ImportAsset(PathW origin, PathW loadTo)
{
	return true;
}

bool Plu::SceneAssetHandler::LoadAssetData(TUsePointer<AssetDescriptor> assetDesc,
	TOwningPointer<IAssetData> *assetDataToPopulate, TUsePointer<EngineAssetManager> assetManager,
	TUsePointer<EngineObjectManager> objectManager, TUsePointer<SceneManager> sceneManager,
	TUsePointer<IShaderManager> shaderManager)
{
	TOwningPointer<SceneInfo> sceneInfo = CreateOwning<SceneInfo>();
	sceneInfo->URL = assetDesc->AssetPath.GetStem();
	std::optional<JSON> j = DiskManager::LoadJson(assetDesc->AssetPath.ToString().ToWide());
	if (!j.has_value()) return false;
	sceneInfo->Uuid = j.value()["uuid"].get<UInt64>();
	*assetDataToPopulate = sceneInfo;
	TUsePointer<SceneManager> editorScenesManager = DynamicCast<SceneManager>(sceneManager);
	editorScenesManager->RegisterSceneInfo(sceneInfo);
	return true;
}

Plu::TypeInfo * Plu::SceneAssetHandler::GetAssetTypeViewportClass()
{
	return SceneViewport::GetStaticClass();
}

bool Plu::SceneAssetHandler::DispatchAssetSave(TUsePointer<AssetDescriptor> assetDesc,
	TUsePointer<EngineAssetManager> assetManager, TUsePointer<EngineObjectManager> objectManager,
	TUsePointer<SceneManager> sceneManager, TUsePointer<IShaderManager> shaderManager)
{
	sceneManager->SaveActiveScene();
	return true;
}
