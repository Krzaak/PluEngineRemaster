//
// Created by Plutex on 5/29/26.
//

#include "PluEngine/Scenes/SceneManager.h"

#include "PluEngine/Application.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"
#include "PluEngine/Scenes/SceneWorld.h"

void Plu::SceneManager::UnloadScene(TUsePointer<SceneWorld> sceneWorld)
{
#ifdef PLU_ENGINE_EDITOR_BUILD
    if (sceneWorld == mOverlayScene || (sceneWorld->Info && sceneWorld->Info->URL == "Overlay")) {
        UnloadOverlayScene();
    }
	if (sceneWorld && sceneWorld == mActivePIEScene) {
		mActivePIEScene->UnloadGameObjects();
		mRenderer->ClearRenderables();
		mRenderer->SetCamera(nullptr);
		mObjectManager->DestroyObject(*mActivePIEScene->GetEngineObjectHandle());
		mActivePIEScene = nullptr;
		if (mActiveScene) {
			mActiveScene->LoadRenderables();
			IRendererCamera* cameraToViewInEditor = nullptr;
			DispatchEvent("EditorCameraWanted", &cameraToViewInEditor);
			mRenderer->SetCamera(cameraToViewInEditor);
		}
	}
#endif
    if (sceneWorld && sceneWorld == mActiveScene) {
        mActiveScene->UnloadGameObjects();
        mRenderer->ClearRenderables();
    	mRenderer->SetCamera(nullptr);
        mObjectManager->DestroyObject(*mActiveScene->GetEngineObjectHandle());
        mActiveScene = nullptr;
    }
}

void Plu::SceneManager::LoadScene(String url, TOwningPointer<SceneWorld>* field, bool play)
{
    if (!mRegisteredScenesByURL.Contains(url)) return;
    TUsePointer<SceneInfo> sceneInfo = mRegisteredScenesByURL[url];
    if (!sceneInfo) return;
    TUsePointer<AssetDescriptor> assetDesc = mAssetManager->GetAssetDescriptor(sceneInfo->Uuid.getUUID());
    if (!assetDesc) return;
    if (*field) UnloadScene(*field);
    mRenderer->ClearRenderables();
	mRenderer->SetCamera(nullptr);
    EngineObjectHandle hdl = mObjectManager->CreateObject<SceneWorld>();
    TOwningPointer<SceneWorld> newWorld = mObjectManager->GetObjectAsOwner<SceneWorld>(hdl);
    newWorld->Init(mObjectManager, mRenderer, mClient);
    newWorld->Info = sceneInfo;
    LoadSceneFromFile(newWorld);
    newWorld->LoadGameObjects();
    newWorld->LoadRenderables();
	*field = newWorld;
	if (play) {
		newWorld->Play();
	} else {
#ifdef PLU_ENGINE_EDITOR_BUILD
		IRendererCamera* cameraToViewInEditor = nullptr;
		DispatchEvent("EditorCameraWanted", &cameraToViewInEditor);
		mRenderer->SetCamera(cameraToViewInEditor);
#endif
	}
	PLU_CORE_INFO("New Scene Loaded! URL {}", url.CStr());
}

#ifdef PLU_ENGINE_EDITOR_BUILD
void Plu::SceneManager::CreateOverlayScene()
{
	mRenderer->ClearRenderables();
	EngineObjectHandle hdl = mObjectManager->CreateObject<SceneWorld>();
	mOverlayScene = mObjectManager->GetObjectAsOwner<SceneWorld>(hdl);
	mOverlayScene->Init(mObjectManager, mRenderer, mClient);
	mOverlayScene->Info = nullptr;
	mOverlayScene->LoadGameObjects();
	mOverlayScene->LoadRenderables();
	IRendererCamera* cameraToViewInEditor = nullptr;
	DispatchEvent("EditorCameraWanted", &cameraToViewInEditor);
	mRenderer->SetCamera(cameraToViewInEditor);
}

void Plu::SceneManager::UnloadOverlayScene()
{
	if (mOverlayScene) {
		mOverlayScene->UnloadGameObjects();
		mRenderer->ClearRenderables();
		mRenderer->SetCamera(nullptr);
		mObjectManager->DestroyObject(*mOverlayScene->GetEngineObjectHandle());
		mOverlayScene = nullptr;
		if (mActiveScene && !IsInPIE()) {
			IRendererCamera* cameraToViewInEditor = nullptr;
			DispatchEvent("EditorCameraWanted", &cameraToViewInEditor);
			mRenderer->SetCamera(cameraToViewInEditor);
		}
	}
}
#endif

Plu::SceneManager::SceneManager()
{

}

Plu::SceneManager::~SceneManager()
{
#ifdef PLU_ENGINE_EDITOR_BUILD
	UnloadScene(mActivePIEScene);
#endif
	UnloadScene(mActiveScene);
}

static Plu::TUsePointer<Plu::SceneManager> gSceneManager;

void Plu::SceneManager::Initialize(ApplicationInfo *appInfo)
{
    mObjectManager = appInfo->AppObjectManager;
    mRenderer = appInfo->AppRenderer;
    mClient = appInfo->Client;
	mShaderManager = appInfo->AppShaderManager;
    mAssetManager = appInfo->AppAssetManager;
    mRegisteredScenesByURL["Overlay"] = nullptr;
    gSceneManager = mObjectManager->GetObjectAsUser<SceneManager>(*this->GetEngineObjectHandle());
}

void Plu::SceneManager::OnUpdate(float deltaTime)
{
    if (GetCurrentWorld()) GetCurrentWorld()->TickScene(deltaTime);
}

Plu::String Plu::SceneManager::GetCurrentWorldName()
{
    if (GetCurrentWorld()) {
        if (GetCurrentWorld()->Info) {
            return GetCurrentWorld()->Info->URL;
        }
    }
    return "";
}

Plu::TUsePointer<Plu::SceneWorld> Plu::SceneManager::GetCurrentWorld()
{
#ifdef PLU_ENGINE_EDITOR_BUILD
    return mOverlayScene ? mOverlayScene : mActivePIEScene ? mActivePIEScene : mActiveScene;
#else
    return mActiveScene;
#endif
}

bool Plu::SceneManager::ConnectToWorld(String URL, bool startPlayOnLoad)
{
#ifdef PLU_ENGINE_EDITOR_BUILD
    UnloadOverlayScene();
    if (URL == "Overlay") {
        CreateOverlayScene();
        return true;
    }
    if (IsInPIE()) return false;
    if (GetCurrentWorldName() == URL) return false;
    UnloadScene(GetCurrentWorld());
    LoadScene(URL, &mActiveScene, startPlayOnLoad);
    return true;
#else
    if (GetCurrentWorldName() == URL) return false;
    UnloadScene(GetCurrentWorld());
    LoadScene(URL, &mActiveScene);
    return true;
#endif
}

bool Plu::SceneManager::IsAnySceneOpen()
{
    return GetCurrentWorld().IsValid();
}

void Plu::SceneManager::RegisterSceneInfo(TUsePointer<SceneInfo> sceneInfo)
{
    sceneInfo->URL.Replace(" ","");
    if (sceneInfo->URL == "Overlay" || sceneInfo->URL == "ActiveScene") {
        return;
    }
    mRegisteredScenesByURL[sceneInfo->URL] = sceneInfo;
}

#ifdef PLU_ENGINE_EDITOR_BUILD
bool Plu::SceneManager::EnterPIE()
{
	if (mIsInPIE) return false;
	mIsInPIE = true;
	UnloadOverlayScene();
	SaveActiveScene();
	LoadScene(GetCurrentWorldName(), &mActivePIEScene, true);
	return true;
}

void Plu::SceneManager::ExitPIE()
{
	if (!mIsInPIE) return;
	mIsInPIE = false;
	UnloadScene(mActivePIEScene);
}

bool Plu::SceneManager::IsInPIE() const
{
	return mActivePIEScene && mIsInPIE;
}
#endif

Plu::TUsePointer<Plu::SceneWorld> Plu::GetCurrentWorld()
{
    return gSceneManager ? gSceneManager->GetCurrentWorld() : nullptr;
}

void Plu::SceneManager::DeserializeWorldComponent(JSON j, TUsePointer<WorldComponent> parentComponent, TUsePointer<GameObject> parentObject)
{
	DeserializationContext* dc = new DeserializationContext();
	dc->assetManager = mAssetManager;
	dc->scenesManager = mObjectManager->GetObjectAsUser<SceneManager>(*this->GetEngineObjectHandle());
	dc->shaderManager = mShaderManager;
	TypeInfo* componentClass = TypeRegistry::GetInstance()->GetTypeOfName(j["typeName"].get<std::string>().c_str());
	TUsePointer<WorldComponent> tmpWorldComponent = mObjectManager->CreateObject(WorldComponent::GetStaticClass());
	TypeSerializer<TypeInfo*>::Deserialize(dc, j, WorldComponent::GetStaticClass(), tmpWorldComponent.GetRaw());
	String componentName = tmpWorldComponent->GetComponentName();
	mObjectManager->DestroyObject(*tmpWorldComponent->GetEngineObjectHandle());
	DynamicArray<TUsePointer<WorldComponent>> componentsToSearchIn = parentComponent ? parentComponent->GetChildren() : parentObject->GetDirectlyAttachedWorldComponents();
	TUsePointer<WorldComponent>* result = componentsToSearchIn.FindIf([componentName](TUsePointer<WorldComponent> comp) -> bool {
		return componentName == comp->GetComponentName();
	});
	if (result != componentsToSearchIn.End()) {
		// TypeSerializer<TypeInfo*>::Deserialize(dc, j, componentClass, result->GetRaw());
		//
		// if (j.contains("relativeLocation")) {
		// 	Vec3 newRelativeLocation;
		// 	TypeSerializer<Vec3>::Deserialize(dc, j["relativeLocation"], &newRelativeLocation);
		// 	result->GetRaw()->SetRelativeLocation(newRelativeLocation);
		// }
		//
		// if (j.contains("relativeRotation")) {
		// 	Vec3 newRelativeRotation;
		// 	TypeSerializer<Vec3>::Deserialize(dc, j["relativeRotation"], &newRelativeRotation);
		// 	result->GetRaw()->SetRelativeRotation(newRelativeRotation);
		// }
		//
		// if (j.contains("relativeScale")) {
		// 	Vec3 newRelativeScale;
		// 	TypeSerializer<Vec3>::Deserialize(dc, j["relativeScale"], &newRelativeScale);
		// 	result->GetRaw()->SetRelativeScale(newRelativeScale);
		// }

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

#ifdef PLU_ENGINE_EDITOR_BUILD
void Plu::SceneManager::SaveActiveScene()
{
	PathW scenePath = mAssetManager->GetAssetPath(mActiveScene->Info->Uuid).ToString().ToWide();
	nlohmann::json json;
	json = DiskManager::LoadJson(scenePath);
	json["gameModeClass"] = mActiveScene->GameModeClass.GetRawType()->TypeName.CStr();
	json["gameObjects"].clear();
#ifdef PLU_ENGINE_EDITOR_BUILD
	Vec3 cameraLoc;
	DispatchEvent("EditorCameraLocationToSave", &cameraLoc);
	Vec3 cameraRot;
	DispatchEvent("EditorCameraRotationToSave", &cameraRot);
	json["editorCameraLocation"] = TypeSerializer<Vec3>::Serialize(&cameraLoc);
	json["editorCameraRotation"] = TypeSerializer<Vec3>::Serialize(&cameraRot);
#endif
	auto gameObjects = mActiveScene->GetAllGameObjects();
	for (const auto& gameObject : gameObjects) {
		json["gameObjects"].push_back(TypeSerializer<TUsePointer<GameObject>>::Serialize(const_cast<TUsePointer<GameObject>*>(&gameObject)));
	}
	DiskManager::SaveJson(scenePath.ToString(), json);
}
#endif

void Plu::SceneManager::LoadSceneFromFile(TUsePointer<SceneWorld> sceneWorld)
{
	JSON j = DiskManager::LoadJson(mAssetManager->GetAssetPath(sceneWorld->Info->Uuid).ToString().ToWide());
	if (j.contains("gameModeClass")) {
		sceneWorld->GameModeClass = TypeRegistry::GetInstance()->GetTypeOfName(j["gameModeClass"].get<std::string>().c_str());
	}
	if (j.contains("editorCameraLocation") && j.contains("editorCameraRotation")) {
		Vec3 location;
		Vec3 rotation;
		TypeSerializer<Vec3>::Deserialize(nullptr, j["editorCameraLocation"], &location);
		TypeSerializer<Vec3>::Deserialize(nullptr, j["editorCameraRotation"], &rotation);
		DispatchEvent("EditorCameraLocationLoaded", &location);
		DispatchEvent("EditorCameraRotationLoaded", &rotation);
	}
	for (auto obj : j["gameObjects"]) {
		LoadGameObjectFromJSON(sceneWorld, obj);
	}
}

void Plu::SceneManager::LoadGameObjectFromJSON(TUsePointer<SceneWorld> sceneWorld, JSON j)
{
	DeserializationContext* dc = new DeserializationContext();
	dc->assetManager = mAssetManager;
	dc->scenesManager = mObjectManager->GetObjectAsUser<SceneManager>(*this->GetEngineObjectHandle());
	dc->shaderManager = mShaderManager;
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
		TUsePointer<GameObjectComponent> component = mObjectManager->CreateObject(GameObjectComponent::GetStaticClass());
		TypeSerializer<TypeInfo*>::Deserialize(dc, comp, component->GetClass(), component.GetRaw());
		TOwningPointer<GameObjectComponent>* findComp = gameObject->GetObjectComponents()->FindIf([component](TOwningPointer<GameObjectComponent> find)->bool {
			if (find->GetComponentName() == component->GetComponentName()) {
				return true;
			}
			return false;
		});
		mObjectManager->DestroyObject(*component->GetEngineObjectHandle());
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