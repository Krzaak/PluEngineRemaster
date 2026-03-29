//
// Created by Plutex on 1/11/26.
//

#include "EditorScenesManager.h"
#include "EditorAppContext.h"
#include "EditorCamera.h"
#include "json_fwd.hpp"
#include "Managers/Assets/EditorAssetManager.h"
#include "Managers/Assets/EditorAssetObject.h"
#include "Managers/Project/EditorProjectManager.h"
#include "PluEngine/Application.h"
#include "PluEngine/PluPaths.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "Managers/Shaders/EditorShaderManager.h"
#include "PluEngine/GameCore/GameMode.h"
#include "PluEngine/GameObject/WorldComponent.h"
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
		sceneToLoad->Init(mEngineObjectManager, gApplicationInfo->AppRenderer, gApplicationInfo->Client);
		sceneToLoad->Info = mRegisteredScenes[url]->AssetInfo;
		if (sceneToUnload) {
			sceneToLoad->GameModeClass = sceneToUnload->GameModeClass;
		}
	} else {
		sceneToUnload = mActivePIEScene;
		sceneToLoad = mActiveScene;
	}
	//Unload previous scene
	if (sceneToUnload && !pie) {
		sceneToUnload->UnloadGameObjects();
		mEngineObjectManager->DestroyObject(*sceneToUnload->GetEngineObjectHandle());
	}
	if (!exitPie) {
		LoadSceneFromFile(sceneToLoad);
		if (!SceneCamera) {
			SceneCamera = mEngineObjectManager->CreateObject(EditorSceneCamera::GetStaticClass());
		}
		sceneToLoad->LoadGameObjects();
		if (!editor) {
			sceneToLoad->Play();
		} else {
			gApplicationInfo->AppRenderer->SetCamera(SceneCamera.GetRaw());
		}
	} else {
		gApplicationInfo->AppRenderer->SetCamera(SceneCamera.GetRaw());
		mActivePIEScene = nullptr;
	}
	if (pie || exitPie) {
		gApplicationInfo->AppRenderer->ClearRenderables();
		sceneToLoad->LoadRenderables();
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

void Plu::EditorScenesManager::DeserializeWorldComponent(JSON j, TUsePointer<WorldComponent> parentComponent, TUsePointer<GameObject> parentObject)
{
	DeserializationContext* dc = new DeserializationContext();
	dc->assetManager = gApplicationInfo->AppAssetManager;
	dc->scenesManager = gApplicationInfo->AppScenesManager;
	dc->shaderManager = gApplicationInfo->AppShaderManager;
	TypeInfo* componentClass = TypeRegistry::GetInstance()->GetTypeOfName(j["typeName"].get<std::string>().c_str());
	TUsePointer<WorldComponent> tmpWorldComponent = mEngineObjectManager->CreateObject(WorldComponent::GetStaticClass());
	TypeSerializer<TypeInfo*>::Deserialize(dc, j, WorldComponent::GetStaticClass(), tmpWorldComponent.GetRaw());
	String componentName = tmpWorldComponent->GetComponentName();
	mEngineObjectManager->DestroyObject(*tmpWorldComponent->GetEngineObjectHandle());
	DynamicArray<TUsePointer<WorldComponent>> componentsToSearchIn = parentComponent ? parentComponent->GetChildren() : parentObject->GetDirectlyAttachedWorldComponents();
	TUsePointer<WorldComponent>* result = componentsToSearchIn.FindIf([componentName](TUsePointer<WorldComponent> comp) -> bool {
		return componentName == comp->GetComponentName();
	});
	if (result) {
		TypeSerializer<TypeInfo*>::Deserialize(dc, j, componentClass, result->GetRaw());
		if (!j.contains("children")) return;
		for (auto child : j["children"]) {
			DeserializeWorldComponent(child, *result, parentObject);
		}
		return;
	}
	TUsePointer<WorldComponent> newComponent = parentObject->AddComponent(componentClass, componentName);
	TypeSerializer<TypeInfo*>::Deserialize(dc, j, componentClass, newComponent.GetRaw());
	if (!j.contains("children")) return;
	for (auto child : j["children"]) {
		DeserializeWorldComponent(child, newComponent, parentObject);
	}
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
	InitSceneManagerForPython(engineObjectManager->GetObjectAsUser<IScenesManager>(*GetEngineObjectHandle()));
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
	json["gameModeClass"] = mActiveScene->GameModeClass.GetRawType()->TypeName.CStr();
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
	if (j.contains("gameModeClass")) {
		sceneWorld->GameModeClass = TypeRegistry::GetInstance()->GetTypeOfName(j["gameModeClass"].get<std::string>().c_str());
	}
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
	if (!gameObject) {
		PLU_ERROR("No GameObject class of name {}! Maybe some python scripts were not run!", j["typeName"].get<std::string>().c_str());
		return;
	}
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
		DeserializeWorldComponent(worldComp, nullptr, gameObject);
	}

	for (auto comp : j["components"]) {
		TUsePointer<GameObjectComponent> component = mEngineObjectManager->CreateObject(GameObjectComponent::GetStaticClass());
		TypeSerializer<TypeInfo*>::Deserialize(dc, comp, component->GetClass(), component.GetRaw());
		TOwningPointer<GameObjectComponent>* findComp = gameObject->GetObjectComponents()->FindIf([component](TOwningPointer<GameObjectComponent> find)->bool {
			if (find->GetComponentName() == component->GetComponentName()) {
				return true;
			}
			return false;
		});
		mEngineObjectManager->DestroyObject(*component->GetEngineObjectHandle());
		component = nullptr;
		if (findComp != gameObject->GetObjectComponents()->End()) {
			TOwningPointer<WorldComponent> compToPopulate = *findComp;
			TypeSerializer<TypeInfo*>::Deserialize(dc, comp, compToPopulate->GetClass(), compToPopulate.GetRaw());
			continue;
		}
		component = gameObject->AddComponent(TypeRegistry::GetInstance()->GetTypeOfName(comp["typeName"].get<std::string>().c_str()), "comp");
		if (!component) continue;
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
