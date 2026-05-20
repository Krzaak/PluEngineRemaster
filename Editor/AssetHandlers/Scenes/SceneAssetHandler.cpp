//
// Created by Plutex on 1/12/26.
//

#include "SceneAssetHandler.h"

#include "EditorAppContext.h"
#include "json_fwd.hpp"
#include "DefinedViewports/Scene/SceneViewport.h"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Project/EditorProjectManager.h"
#include "Managers/Scene/EditorScenesManager.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectHandle.h"
#include "PluEngine/Objects/EngineObjectManager.h"

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

Plu::TUsePointer<Plu::IEditorAssetObject> Plu::SceneAssetHandler::LoadAsset(
	PathW path, TUsePointer<EditorProjectManager> editorProjectManager,
	TUsePointer<EngineObjectManager> engineObjectManager, EditorAssetManager *editorAssetManager)
{
	// EngineObjectHandle assetObject = engineObjectManager->CreateObject<EditorAssetObject<SceneInfo>>();
	// TOwningPointer<EditorAssetObject<SceneInfo>> assetObjectT = engineObjectManager->GetObjectAsOwner<EditorAssetObject<SceneInfo>>(assetObject);
	// assetObjectT->AssetInfo->URL = path.GetStem().ToNarrow();
	// //editorAssetManager->AddAssetFromHandler(assetObjectT, assetObjectT->AssetInfo->Uuid, path, SceneInfo::GetStaticClass());
	// editorProjectManager->GetAppContext()->EditorScenesManager->AddSceneInfo(assetObjectT->AssetInfo->URL, assetObjectT);
	return nullptr;
}

Plu::TypeInfo * Plu::SceneAssetHandler::GetAssetViewportClass()
{
	return SceneViewport::GetStaticClass();
}
