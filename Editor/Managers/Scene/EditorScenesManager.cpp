//
// Created by Plutex on 1/11/26.
//

#include "EditorScenesManager.h"

#include "EditorAppContext.h"
#include "EditorScene.h"
#include "json_fwd.hpp"
#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

bool Plu::EditorScenesManager::OpenSceneInternal(const String& url, bool editor)
{
	if (mActiveScene) {
		if (mActiveScene->Info.URL == url) {
			return false;
		}
	}
	TOwningPointer<EditorScene> sceneToLoad = mEngineObjectManager->CreateObject(EditorScene::GetStaticClass());
	sceneToLoad->Init(mEngineObjectManager);
	sceneToLoad->Info = mRegisteredScenes[url]->AssetInfo;
	//Unload previous scene
	if (mActiveScene) {
		mActiveScene->UnloadGameObjects();
		mEngineObjectManager->DestroyObject(*mActiveScene->GetEngineObjectHandle());
	}
	sceneToLoad->LoadGameObjects();
	if (!editor)
		sceneToLoad->Play();
	mActiveScene = sceneToLoad;

	return true;
}

void Plu::EditorScenesManager::AddSceneInfo(const String& name, const TUsePointer<EditorAssetObject<SceneInfo>> &sceneAsset)
{
	mRegisteredScenes.Insert(name, sceneAsset);
	PLU_INFO("Registered scene: {} at: {}", name.CStr(), sceneAsset->GetAssetPath().ToString().ToNarrow().CStr());
}

Plu::EditorScenesManager::EditorScenesManager()
= default;

Plu::EditorScenesManager::~EditorScenesManager()
{
}

void Plu::EditorScenesManager::CreateNewScene(const String& name, PathW path)
{
	path += L"/";
	path += name.ToWide();
	path += PLU_SCENE_EXT_W;

	nlohmann::json json = {
		{"uuid", PluUUID().getUUID()},
		{"type", "Scene"}
	};
	json["gameObjects"] = nlohmann::json::array();
	DiskManager::SaveJson(path.ToString(), json);
}

void Plu::EditorScenesManager::Init(const TUsePointer<EditorProjectManager> &editorProjectManager,
                                    const TUsePointer<EngineObjectManager> &engineObjectManager)
{
	mEditorProjectManager = editorProjectManager;
	mEngineObjectManager = engineObjectManager;
}

void Plu::EditorScenesManager::Shutdown()
{
	if (mActiveScene) {
		mActiveScene->UnloadGameObjects();
		mEngineObjectManager->DestroyObject(*mActiveScene->GetEngineObjectHandle());
	}
}

bool Plu::EditorScenesManager::ConnectToWorld(String URL)
{
	if (mRegisteredScenes.Contains(URL)) {
		return OpenSceneInternal(URL, false);
	}
	return false;
}

bool Plu::EditorScenesManager::PrepareWorldForEditor(String URL)
{
	PLU_INFO("Load Scene for Editor called for: {}", URL.CStr());
	if (mRegisteredScenes.Contains(URL)) {
		PLU_INFO("Found scene with URL: {}", URL.CStr());
		return OpenSceneInternal(URL, true);
	}
	return false;
}

Plu::String Plu::EditorScenesManager::GetCurrentWorldName()
{
	if (mActiveScene) {
		return mActiveScene->Info.URL;
	}
	return "";
}

bool Plu::EditorScenesManager::IsAnySceneOpen()
{
	return mActiveScene;
}
