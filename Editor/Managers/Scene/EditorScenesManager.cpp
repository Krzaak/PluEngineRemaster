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

void Plu::EditorScenesManager::UnloadOverlayScene(bool loadBackActive)
{
	if (mOverlayScene) {
		mOverlayScene->UnloadGameObjects();
		gApplicationInfo->AppRenderer->ClearRenderables();
		mEngineObjectManager->DestroyObject(*mOverlayScene->GetEngineObjectHandle());
		mOverlayScene = nullptr;
	}
	if (loadBackActive) {
		if (GetCurrentWorld())
		{
			gApplicationInfo->AppRenderer->ClearRenderables();
			GetCurrentWorld()->LoadRenderables();
		}
	}
}

bool Plu::EditorScenesManager::OpenSceneInternal(const String& url, bool editor, bool pie, bool exitPie)
{
	if (mActiveScene) {
		if (mActiveScene->Info->URL == url && !pie && !exitPie) {
			return false;
		}
	}
	bool isOverlayScene = url == "EditorScene";
	UnloadOverlayScene(!isOverlayScene);
	if (isOverlayScene) {
		PLU_INFO("Loading Overlay Scene");
		gApplicationInfo->AppRenderer->SetCamera(SceneCamera.GetRaw());
		gApplicationInfo->AppRenderer->ClearRenderables();
		EngineObjectHandle hdl = mEngineObjectManager->CreateObject<SceneWorld>();
		mOverlayScene = mEngineObjectManager->GetObjectAsOwner<SceneWorld>(hdl);
		mOverlayScene->Init(mEngineObjectManager, gApplicationInfo->AppRenderer, gApplicationInfo->Client);
		mOverlayScene->Info = nullptr;
		mOverlayScene->LoadGameObjects();
		mOverlayScene->LoadRenderables();
		return true;
	}
	TOwningPointer<SceneWorld> sceneToLoad;
	TUsePointer<SceneWorld> sceneToUnload = mActiveScene;
	if (!exitPie) {
		EngineObjectHandle hdl = mEngineObjectManager->CreateObject<SceneWorld>();
		sceneToLoad = mEngineObjectManager->GetObjectAsOwner<SceneWorld>(hdl);
		sceneToLoad->Init(mEngineObjectManager, gApplicationInfo->AppRenderer, gApplicationInfo->Client);
		sceneToLoad->Info = mRegisteredScenes[url]->GetAssetInfoPtr();
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
		if (!SceneCamera) {
			EngineObjectHandle hdlCamera = mEngineObjectManager->CreateObject<EditorSceneCamera>();
			SceneCamera = mEngineObjectManager->GetObjectAsOwner<EditorSceneCamera>(hdlCamera);
		}
		LoadSceneFromFile(sceneToLoad);
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
	if (result != componentsToSearchIn.End()) {
		TypeSerializer<TypeInfo*>::Deserialize(dc, j, componentClass, result->GetRaw());

		if (j.contains("relativeLocation")) {
			Vec3 newRelativeLocation;
			TypeSerializer<Vec3>::Deserialize(dc, j["relativeLocation"], &newRelativeLocation);
			result->GetRaw()->SetRelativeLocation(newRelativeLocation);
		}

		if (j.contains("relativeRotation")) {
			Vec3 newRelativeRotation;
			TypeSerializer<Vec3>::Deserialize(dc, j["relativeRotation"], &newRelativeRotation);
			result->GetRaw()->SetRelativeRotation(newRelativeRotation);
		}

		if (j.contains("relativeScale")) {
			Vec3 newRelativeScale;
			TypeSerializer<Vec3>::Deserialize(dc, j["relativeScale"], &newRelativeScale);
			result->GetRaw()->SetRelativeScale(newRelativeScale);
		}

		if (!j.contains("children")) return;
		for (auto child : j["children"]) {
			DeserializeWorldComponent(child, *result, parentObject);
		}
		return;
	}
	TUsePointer<WorldComponent> newComponent = parentObject->AddComponent(componentClass, componentName);
	TypeSerializer<TypeInfo*>::Deserialize(dc, j, componentClass, newComponent.GetRaw());

	if (j.contains("relativeLocation")) {
		Vec3 newRelativeLocation;
		TypeSerializer<Vec3>::Deserialize(dc, j["relativeLocation"], &newRelativeLocation);
		newComponent->SetRelativeLocation(newRelativeLocation);
	}

	if (j.contains("relativeRotation")) {
		Vec3 newRelativeRotation;
		TypeSerializer<Vec3>::Deserialize(dc, j["relativeRotation"], &newRelativeRotation);
		newComponent->SetRelativeRotation(newRelativeRotation);
	}

	if (j.contains("relativeScale")) {
		Vec3 newRelativeScale;
		TypeSerializer<Vec3>::Deserialize(dc, j["relativeScale"], &newRelativeScale);
		newComponent->SetRelativeScale(newRelativeScale);
	}

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
	mRegisteredScenes.Insert("EditorScene", nullptr);
}

void Plu::EditorScenesManager::Shutdown()
{
	gApplicationInfo->AppRenderer->ClearRenderables();
	if (mActiveScene) {
		SaveActiveScene();
		mEngineObjectManager->DestroyObject(*SceneCamera->GetEngineObjectHandle());
		SceneCamera = nullptr;
		mActiveScene->UnloadGameObjects();
		mEngineObjectManager->DestroyObject(*mActiveScene->GetEngineObjectHandle());
		mActiveScene = nullptr;
	}
	if (mActivePIEScene) {
		mActivePIEScene->UnloadGameObjects();
		mEngineObjectManager->DestroyObject(*mActivePIEScene->GetEngineObjectHandle());
		mActivePIEScene = nullptr;
	}
	if (mOverlayScene) {
		mOverlayScene->UnloadGameObjects();
		mEngineObjectManager->DestroyObject(*mOverlayScene->GetEngineObjectHandle());
		mOverlayScene = nullptr;
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
	Vec3 location = SceneCamera->GetCameraLocation();
	Vec3 rotation = SceneCamera->GetNiceRotation();
	json["editorCameraLocation"] = TypeSerializer<Vec3>::Serialize(&location);
	json["editorCameraRotation"] = TypeSerializer<Vec3>::Serialize(&rotation);
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
	if (j.contains("editorCameraLocation") && j.contains("editorCameraRotation") && SceneCamera) {
		Vec3 location;
		Vec3 rotation;
		TypeSerializer<Vec3>::Deserialize(nullptr, j["editorCameraLocation"], &location);
		TypeSerializer<Vec3>::Deserialize(nullptr, j["editorCameraRotation"], &rotation);
		SceneCamera->SetLocation(location);
		SceneCamera->SetRotation(rotation);
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
	TypeSerializer<TypeInfo*>::Deserialize(dc, j, gameObject->GetClass(), gameObject.GetRaw());
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
	return mActivePIEScene ? mActivePIEScene : mOverlayScene ? mOverlayScene : mActiveScene;
}

Plu::TUsePointer<Plu::SceneWorld> Plu::EditorScenesManager::CreateOverlayWorld()
{
	OpenSceneInternal("EditorScene", true);
	return mOverlayScene;
}

Plu::TUsePointer<Plu::SceneWorld> Plu::EditorScenesManager::GetCurrentEditorScene()
{
	return mActivePIEScene ? mActivePIEScene : mOverlayScene ? mOverlayScene : mActiveScene;
}
