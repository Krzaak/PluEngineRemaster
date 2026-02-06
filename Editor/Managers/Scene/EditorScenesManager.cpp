//
// Created by Plutex on 1/11/26.
//

#include "EditorScenesManager.h"

#include "EditorAppContext.h"
#include "json_fwd.hpp"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Reflection/TypeTraits.h"
#include "Managers/Shaders/EditorShaderManager.h"

extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::ApplicationInfo* gApplicationInfo;

bool Plu::EditorScenesManager::OpenSceneInternal(const String& url, bool editor)
{
	if (mActiveScene) {
		if (mActiveScene->Info->URL == url) {
			return false;
		}
	}
	TOwningPointer<SceneWorld> sceneToLoad = mEngineObjectManager->CreateObject(SceneWorld::GetStaticClass());
	sceneToLoad->Init(mEngineObjectManager, gApplicationInfo->AppRenderer);
	sceneToLoad->Info = mRegisteredScenes[url]->AssetInfo;
	//Unload previous scene
	if (mActiveScene) {
		mActiveScene->UnloadGameObjects();
		mEngineObjectManager->DestroyObject(*mActiveScene->GetEngineObjectHandle());
	}
	LoadSceneFromFile(sceneToLoad);
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
		{"typeName", "SceneInfo"}
	};
	json["gameObjects"] = nlohmann::json::array();
	DiskManager::SaveJson(path.ToString(), json);
	gEditorAppContext->EditorAssetManager->LoadAsset(path.ToString());
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
		SaveActiveScene();
		mActiveScene->UnloadGameObjects();
		mEngineObjectManager->DestroyObject(*mActiveScene->GetEngineObjectHandle());
		mActiveScene = nullptr;
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
		return mActiveScene->Info->URL;
	}
	return "";
}

bool Plu::EditorScenesManager::IsAnySceneOpen()
{
	return mActiveScene;
}

void Plu::EditorScenesManager::SaveActiveScene()
{
	PathW scenePath = gEditorAppContext->EditorAssetManager->GetAssetPathByUUID(mActiveScene->Info->Uuid);
	nlohmann::json json;
	json = DiskManager::LoadJson(scenePath);
	json["gameObjects"].clear();
	auto gameObjects = mActiveScene->GetAllGameObjects();
	for (const auto& gameObject : gameObjects) {
		json["gameObjects"].push_back(TypeSerializer<TUsePointer<GameObject>>::Serialize(const_cast<TUsePointer<GameObject>*>(&gameObject)));
	}
	DiskManager::SaveJson(scenePath.ToString(), json);
}

void Plu::EditorScenesManager::LoadSceneFromFile(TUsePointer<SceneWorld> sceneWorld)
{
	JSON j = DiskManager::LoadJson(gEditorAppContext->EditorAssetManager->GetAssetPathByUUID(sceneWorld->Info->Uuid));
	for (auto obj : j["gameObjects"]) {
		DeserializationContext* dc = new DeserializationContext();
		dc->assetManager = gEditorAppContext->EditorAssetManager;
		dc->scenesManager = gEditorAppContext->EditorScenesManager;
		dc->shaderManager = gEditorAppContext->EditorShaderManager;
		TUsePointer<GameObject> gameObject = sceneWorld->SpawnGameObject(TypeRegistry::GetInstance()->GetTypeOfName(obj["typeName"].get<std::string>().c_str()));
		Vec3 loc;
		Vec3 rot;
		Vec3 scl;
		TypeSerializer<Vec3>::Deserialize(dc, obj["location"], &loc);
		TypeSerializer<Vec3>::Deserialize(dc, obj["rotation"], &rot);
		TypeSerializer<Vec3>::Deserialize(dc, obj["scale"], &scl);
		gameObject->SetObjectLocation(loc);
		gameObject->SetObjectRotation(rot);
		gameObject->SetObjectScale(scl);
		for (auto worldComp : obj["worldComponents"]) {
			TUsePointer<WorldComponent> worldComponent = gameObject->AddComponent(TypeRegistry::GetInstance()->GetTypeOfName(worldComp["typeName"].get<std::string>().c_str()));
			TypeSerializer<TypeInfo*>::Deserialize(dc, worldComp, worldComponent->GetClass(), worldComponent.GetRaw());
		}
		delete dc;
	}
}

Plu::TUsePointer<Plu::SceneWorld> Plu::EditorScenesManager::GetCurrentEditorScene()
{
	return mActiveScene;
}
