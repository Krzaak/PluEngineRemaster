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
#include "PluEngine/GameCore/GameMode.h"
#include "PluEngine/Renderer/Renderer.h"

extern Plu::EditorAppContext* gEditorAppContext;
extern Plu::ApplicationInfo* gApplicationInfo;

bool Plu::EditorScenesManager::OpenSceneInternal(const String& url, bool editor, bool pie, bool exitPie)
{
	if (mActiveScene) {
		if (mActiveScene->Info->URL == url && !pie && !exitPie) {
			return false;
		}
	}
	TOwningPointer<SceneWorld> sceneToLoad;
	TUsePointer<SceneWorld> sceneToUnload = mActiveScene;
	if (!exitPie) {
		sceneToLoad = mEngineObjectManager->CreateObject(SceneWorld::GetStaticClass());
		sceneToLoad->Init(mEngineObjectManager, gApplicationInfo->AppRenderer);
		sceneToLoad->Info = mRegisteredScenes[url]->AssetInfo;
	} else {
		sceneToUnload = mActivePIEScene;
		sceneToLoad = mActiveScene;
	}
	//Unload previous scene
	if (sceneToUnload && !pie) {
		sceneToUnload->UnloadGameObjects();
		mEngineObjectManager->DestroyObject(*sceneToUnload->GetEngineObjectHandle());
	}
	if (pie) {
		gApplicationInfo->AppRenderer->ClearRenderables();
	}
	if (!exitPie) {
		LoadSceneFromFile(sceneToLoad);
		sceneToLoad->LoadGameObjects();
		if (!editor)
			sceneToLoad->Play();
	} else {
		mActiveScene->LoadRenderables();
		mActivePIEScene = nullptr;
	}
	if (!pie) {
		mActiveScene = sceneToLoad;
	} else {
		mActivePIEScene = sceneToLoad;
	}

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
	if (mActivePIEScene) {
		mActivePIEScene->UnloadGameObjects();
		mEngineObjectManager->DestroyObject(*mActivePIEScene->GetEngineObjectHandle());
		mActivePIEScene = nullptr;
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

Plu::TUsePointer<Plu::SceneWorld> Plu::EditorScenesManager::EnterPIE()
{
	if (!mActiveScene) return nullptr;
	SaveActiveScene();
	OpenSceneInternal(mActiveScene->Info->URL, false, true);
	gEditorAppContext->EditorState.SelectedGameObject = EngineObjectHandle();
	gEditorAppContext->EditorState.SelectedGameObjectComponent = EngineObjectHandle();
	return mActivePIEScene;
}

void Plu::EditorScenesManager::ExitPIE()
{
	if (!mActivePIEScene) return;
	OpenSceneInternal(mActiveScene->Info->URL, false, false, true);
	gEditorAppContext->EditorState.SelectedGameObject = EngineObjectHandle();
	gEditorAppContext->EditorState.SelectedGameObjectComponent = EngineObjectHandle();
}

bool Plu::EditorScenesManager::IsInPIE()
{
	return mActivePIEScene;
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
		LoadGameObjectFromJSON(sceneWorld, obj);
	}
}

void Plu::EditorScenesManager::LoadGameObjectFromJSON(TUsePointer<SceneWorld> sceneWorld, JSON j)
{
	DeserializationContext* dc = new DeserializationContext();
	dc->assetManager = gEditorAppContext->EditorAssetManager;
	dc->scenesManager = gEditorAppContext->EditorScenesManager;
	dc->shaderManager = gEditorAppContext->EditorShaderManager;
	TUsePointer<GameObject> gameObject = sceneWorld->SpawnGameObject(TypeRegistry::GetInstance()->GetTypeOfName(j["typeName"].get<std::string>().c_str()));
	Vec3 loc;
	Vec3 rot;
	Vec3 scl;
	TypeSerializer<Vec3>::Deserialize(dc, j["location"], &loc);
	TypeSerializer<Vec3>::Deserialize(dc, j["rotation"], &rot);
	TypeSerializer<Vec3>::Deserialize(dc, j["scale"], &scl);
	gameObject->SetObjectLocation(loc);
	gameObject->SetObjectRotation(rot);
	gameObject->SetObjectScale(scl);
	for (auto worldComp : j["worldComponents"]) {
		TUsePointer<WorldComponent> worldComponent = gameObject->AddComponent(TypeRegistry::GetInstance()->GetTypeOfName(worldComp["typeName"].get<std::string>().c_str()));
		TypeSerializer<TypeInfo*>::Deserialize(dc, worldComp, worldComponent->GetClass(), worldComponent.GetRaw());
	}
	for (auto comp : j["components"]) {
		TUsePointer<GameObjectComponent> component = gameObject->AddComponent(TypeRegistry::GetInstance()->GetTypeOfName(comp["typeName"].get<std::string>().c_str()));
		TypeSerializer<TypeInfo*>::Deserialize(dc, comp, component->GetClass(), component.GetRaw());
	}
	delete dc;
}

void Plu::EditorScenesManager::TickScene(float deltaTime)
{
	if (!mActivePIEScene) return;
	mActivePIEScene->TickScene(deltaTime);
}

Plu::TUsePointer<Plu::SceneWorld> Plu::EditorScenesManager::GetCurrentWorld()
{
	return mActivePIEScene ? mActivePIEScene : mActiveScene;
}

Plu::TUsePointer<Plu::SceneWorld> Plu::EditorScenesManager::GetCurrentEditorScene()
{
	return mActivePIEScene ? mActivePIEScene : mActiveScene;
}
