//
// Created by Plutex on 5/29/26.
//

#include "PluEngine/Scenes/SceneManager.h"

#include "PluEngine/Application.h"
#include "PluEngine/Timer.h"
#include "PluEngine/BasicEngineClasses/GameObjects/Lights/DirectionalLight.h"
#include "PluEngine/Managers/DiskManager.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Physics/PhysicsWorld.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"
#include "PluEngine/Scenes/SceneWorld.h"

void Plu::SceneManager::UnloadScene(TUsePointer<SceneWorld> sceneWorld)
{
	if (!sceneWorld) return;
#ifdef PLU_ENGINE_EDITOR_BUILD
    if (sceneWorld == mOverlayScene || (sceneWorld->Info && sceneWorld->Info->URL == "Overlay")) {
        UnloadOverlayScene();
    }
	if (sceneWorld && sceneWorld == mActivePIEScene) {
		mActivePIEScene->UnloadGameObjects();
		mEditorCamera = nullptr;
		mObjectManager->DestroyObject(*mActivePIEScene->GetEngineObjectHandle());
		mActivePIEScene = nullptr;
		if (mActiveScene) {
			IRendererCamera* cameraToViewInEditor = nullptr;
			DispatchEvent("EditorCameraWanted", &cameraToViewInEditor);
			mEditorCamera = cameraToViewInEditor;
		}
	}
#endif
    if (sceneWorld && sceneWorld == mActiveScene) {
        mActiveScene->UnloadGameObjects();
        mObjectManager->DestroyObject(*mActiveScene->GetEngineObjectHandle());
        mActiveScene = nullptr;
    }
}

void Plu::SceneManager::LoadScene(String url, TOwningPointer<SceneWorld>* field, bool play, TUsePointer<SceneWorld> cloneSource)
{
    PLU_PROFILE_SCOPE("LoadScene");
    if (!mRegisteredScenesByURL.Contains(url)) return;
    TUsePointer<SceneInfo> sceneInfo = mRegisteredScenesByURL[url];
    if (!sceneInfo) return;
    TUsePointer<AssetDescriptor> assetDesc = mAssetManager->GetAssetDescriptor(sceneInfo->Uuid.getUUID());
    if (!assetDesc) return;
    if (*field) UnloadScene(*field);
    EngineObjectHandle hdl = mObjectManager->CreateObject<SceneWorld>();
    TOwningPointer<SceneWorld> newWorld = mObjectManager->GetObjectAsOwner<SceneWorld>(hdl);
    newWorld->Init(mObjectManager, mClient);
    newWorld->Info = sceneInfo;
    if (cloneSource) {
        CloneSceneInto(newWorld, cloneSource);
    } else {
        LoadSceneFromFile(newWorld);
    }
    newWorld->LoadGameObjects();
	*field = newWorld;
	if (play) {
		newWorld->Play();
	} else {
#ifdef PLU_ENGINE_EDITOR_BUILD
		IRendererCamera* cameraToViewInEditor = nullptr;
		DispatchEvent("EditorCameraWanted", &cameraToViewInEditor);
		mEditorCamera = cameraToViewInEditor;
#endif
	}
	PLU_CORE_INFO("New Scene Loaded! URL {}", url.CStr());
}

#ifdef PLU_ENGINE_EDITOR_BUILD
void Plu::SceneManager::CreateOverlayScene()
{
	EngineObjectHandle hdl = mObjectManager->CreateObject<SceneWorld>();
	mOverlayScene = mObjectManager->GetObjectAsOwner<SceneWorld>(hdl);
	mOverlayScene->Init(mObjectManager, mClient);
	mOverlayScene->Info = nullptr;
	mOverlayScene->LoadGameObjects();
	IRendererCamera* cameraToViewInEditor = nullptr;
	DispatchEvent("EditorCameraWanted", &cameraToViewInEditor);
	mEditorCamera = cameraToViewInEditor;

	TUsePointer<DirectionalLight> dirLight = mOverlayScene->SpawnGameObject(DirectionalLight::GetStaticClass());
	dirLight->SetObjectRotation({250,40,0});
}

void Plu::SceneManager::UnloadOverlayScene()
{
	if (mOverlayScene) {
		mOverlayScene->UnloadGameObjects();
		mEditorCamera = nullptr;
		mObjectManager->DestroyObject(*mOverlayScene->GetEngineObjectHandle());
		mOverlayScene = nullptr;
		if (mActiveScene && !IsInPIE()) {
			IRendererCamera* cameraToViewInEditor = nullptr;
			DispatchEvent("EditorCameraWanted", &cameraToViewInEditor);
			mEditorCamera = cameraToViewInEditor;
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
    mClient = appInfo->Client;
	mShaderManager = appInfo->AppShaderManager;
    mAssetManager = appInfo->AppAssetManager;
    mRegisteredScenesByURL["Overlay"] = nullptr;
    gSceneManager = mObjectManager->GetObjectAsUser<SceneManager>(*this->GetEngineObjectHandle());
}

void Plu::SceneManager::OnUpdate(float deltaTime)
{
#ifdef PLU_ENGINE_EDITOR_BUILD
	if (IsInPIE())
	{
		if (GetCurrentWorld()) GetCurrentWorld()->TickScene(deltaTime);
	}
#else
	if (GetCurrentWorld()) GetCurrentWorld()->TickScene(deltaTime);
#endif
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

void Plu::SceneManager::DisconnectFromWorld()
{
	if (GetCurrentWorld()) {
		UnloadScene(GetCurrentWorld());
	}
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
	PLU_PROFILE_SCOPE("PIE Enter");
	mIsInPIE = true;
	UnloadOverlayScene();
	// The PIE world is duplicated from the editor world in memory. It used to go through the scene
	// file, then through an in-memory JSON snapshot; both paid for building a JSON DOM and taking it
	// apart again, which was the bulk of the transition cost. Nothing is written to disk either way —
	// saving is only what the user explicitly asks for (SaveActiveScene).
	LoadScene(GetCurrentWorldName(), &mActivePIEScene, true, mActiveScene);
	mEditorCamera = nullptr;
	mActivePIEScene->GetPhysicsWorld()->PhysicsDebugRenderMode = mActiveScene->GetPhysicsWorld()->PhysicsDebugRenderMode;
	return true;
}

void Plu::SceneManager::ExitPIE()
{
	if (!mIsInPIE) return;
	PLU_PROFILE_SCOPE("PIE Exit");
	mIsInPIE = false;
	UnloadScene(mActivePIEScene);
	IRendererCamera* cameraToViewInEditor = nullptr;
	DispatchEvent("EditorCameraWanted", &cameraToViewInEditor);
	mEditorCamera = cameraToViewInEditor;
}

bool Plu::SceneManager::IsInPIE() const
{
	return mActivePIEScene && mIsInPIE;
}

Plu::TUsePointer<Plu::SceneWorld> Plu::SceneManager::GetBaseSceneWorld()
{
	return mActiveScene;
}

void Plu::SceneManager::SetEditorRenderCamera(IRendererCamera* camera)
{
	mEditorCamera = camera;
}

Plu::IRendererCamera* Plu::SceneManager::GetEditorRenderCamera() const
{
	return mEditorCamera;
}
#endif

Plu::TUsePointer<Plu::SceneWorld> Plu::GetCurrentWorld()
{
    return gSceneManager ? gSceneManager->GetCurrentWorld() : nullptr;
}

// Reflected properties land in the flat "fields" array written by TypeSerializer<TypeInfo*>, so a
// single string field can be read straight out of the JSON — no need to construct an object and run
// a full deserialization just to inspect one value. outFound (optional) distinguishes "field absent"
// from "field present but empty".
static Plu::String ReadStringFieldFromJson(const JSON& j, const char* fieldName, bool* outFound = nullptr)
{
	if (outFound) *outFound = false;
	if (!j.contains("fields")) return {};
	for (const auto& field : j["fields"]) {
		if (!field.contains("name") || !field.contains("value")) continue;
		if (field["name"].get<std::string>() != fieldName) continue;
		Plu::String value;
		Plu::TypeSerializer<Plu::String>::Deserialize(nullptr, field["value"], &value);
		if (outFound) *outFound = true;
		return value;
	}
	return {};
}

void Plu::SceneManager::DeserializeWorldComponent(DeserializationContext* dc, const JSON& j, TUsePointer<WorldComponent> parentComponent, TUsePointer<GameObject> parentObject)
{
	PLU_PROFILE_SCOPE("DeserializeWorldComponent");
	if (!j.contains("typeName")) return;
	TypeInfo* componentClass = TypeRegistry::GetInstance()->GetTypeOfName(j["typeName"].get<std::string>().c_str());
	const String componentName = ReadStringFieldFromJson(j, "mComponentName");
	DynamicArray<TUsePointer<WorldComponent>> componentsToSearchIn = parentComponent ? parentComponent->GetChildren() : parentObject->GetDirectlyAttachedWorldComponents();
	TUsePointer<WorldComponent>* result = componentsToSearchIn.FindIf([&componentName](TUsePointer<WorldComponent> comp) -> bool {
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
		for (const auto& child : j["children"]) {
			DeserializeWorldComponent(dc, child, *result, parentObject);
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
	for (const auto& child : j["children"]) {
		DeserializeWorldComponent(dc, child, newComponent, parentObject);
	}
}

#ifdef PLU_ENGINE_EDITOR_BUILD
JSON Plu::SceneManager::SerializeActiveScene(JSON base)
{
	PLU_PROFILE_SCOPE("SerializeActiveScene");
	base["gameModeClass"] = mActiveScene->GameModeClass.GetRawType()->TypeName.CStr();
	base["gameObjects"].clear();
	Vec3 cameraLoc;
	DispatchEvent("EditorCameraLocationToSave", &cameraLoc);
	Vec3 cameraRot;
	DispatchEvent("EditorCameraRotationToSave", &cameraRot);
	base["editorCameraLocation"] = TypeSerializer<Vec3>::Serialize(&cameraLoc);
	base["editorCameraRotation"] = TypeSerializer<Vec3>::Serialize(&cameraRot);
	auto gameObjects = mActiveScene->GetAllGameObjects();
	for (const auto& gameObject : gameObjects) {
		base["gameObjects"].push_back(TypeSerializer<TUsePointer<GameObject>>::Serialize(const_cast<TUsePointer<GameObject>*>(&gameObject)));
	}
	return base;
}

void Plu::SceneManager::SaveActiveScene()
{
	PLU_PROFILE_SCOPE("SaveActiveScene");
	PathW scenePath = mAssetManager->GetAssetPath(mActiveScene->Info->Uuid).ToString().ToWide();
	// Start from the file so keys of the scene asset that we do not own survive the round-trip.
	nlohmann::json base;
	base = DiskManager::LoadJson(scenePath);
	JSON json = SerializeActiveScene(std::move(base));
	PLU_PROFILE_SCOPE("SaveActiveScene Write");
	DiskManager::SaveJson(scenePath.ToString(), json);
}
#endif

void Plu::SceneManager::LoadSceneFromFile(TUsePointer<SceneWorld> sceneWorld)
{
	PLU_PROFILE_SCOPE("LoadSceneFromFile");
	JSON j;
	{
		PLU_PROFILE_SCOPE("LoadSceneFromFile Parse");
		j = DiskManager::LoadJson(mAssetManager->GetAssetPath(sceneWorld->Info->Uuid).ToString().ToWide());
	}
	LoadSceneFromJson(sceneWorld, j);
}

void Plu::SceneManager::LoadSceneFromJson(TUsePointer<SceneWorld> sceneWorld, const JSON& j)
{
	PLU_PROFILE_SCOPE("LoadSceneFromJson");
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
	if (!j.contains("gameObjects")) return;
	DeserializationContext dc = MakeDeserializationContext();
	for (const auto& obj : j["gameObjects"]) {
		LoadGameObjectFromJSON(&dc, sceneWorld, obj);
	}
}

void Plu::SceneManager::CloneSceneInto(TUsePointer<SceneWorld> target, TUsePointer<SceneWorld> source)
{
	PLU_PROFILE_SCOPE("CloneSceneInto");
	if (!target || !source) return;
	target->GameModeClass = source->GameModeClass;
	// No editor-camera events here, unlike LoadSceneFromJson: that path replays the camera stored in
	// the scene, but a clone's "stored" camera would be the live editor camera itself, so replaying
	// it would only move the camera to where it already is.
	DynamicArray<TUsePointer<GameObject>> sourceObjects = source->GetAllGameObjects();
	for (const auto& sourceObject : sourceObjects) {
		CloneGameObjectInto(target, sourceObject);
	}
}

void Plu::SceneManager::CloneGameObjectInto(TUsePointer<SceneWorld> targetWorld, TUsePointer<GameObject> source)
{
	PLU_PROFILE_SCOPE("CloneGameObject");
	if (!source) return;
	// Unnamed: the name is a reflected property, so it arrives with the property copy below — same
	// reasoning as the JSON path, where generating a default name would be wasted work.
	TUsePointer<GameObject> clone = targetWorld->SpawnGameObjectUnnamed(source->GetClass());
	if (!clone) return;

	CopyReflectedProperties(source->GetClass(), source.GetRaw(), clone.GetRaw());
	clone->SetObjectLocation(source->GetObjectLocation());
	clone->SetObjectRotation(source->GetObjectRotation());
	clone->SetObjectScale(source->GetObjectScale());

	for (const auto& worldComp : source->GetDirectlyAttachedWorldComponents()) {
		CloneWorldComponent(worldComp, nullptr, clone);
	}

	DynamicArray<TOwningPointer<GameObjectComponent>>* cloneComponents = clone->GetObjectComponents();
	for (const auto& sourceComp : *source->GetObjectComponents()) {
		const String componentName = sourceComp->GetComponentName();
		TOwningPointer<GameObjectComponent>* existing = cloneComponents->FindIf([&componentName](TOwningPointer<GameObjectComponent> find)->bool {
			return find->GetComponentName() == componentName;
		});
		if (existing != cloneComponents->End()) {
			CopyReflectedProperties(sourceComp->GetClass(), sourceComp.GetRaw(), existing->GetRaw());
			continue;
		}
		TUsePointer<GameObjectComponent> newComponent = clone->AddComponent(sourceComp->GetClass(), "comp");
		if (!newComponent) continue;
		CopyReflectedProperties(sourceComp->GetClass(), sourceComp.GetRaw(), newComponent.GetRaw());
	}
}

void Plu::SceneManager::CloneWorldComponent(TUsePointer<WorldComponent> source, TUsePointer<WorldComponent> parentComponent, TUsePointer<GameObject> targetObject)
{
	PLU_PROFILE_SCOPE("CloneWorldComponent");
	if (!source) return;
	const String componentName = source->GetComponentName();
	DynamicArray<TUsePointer<WorldComponent>> componentsToSearchIn = parentComponent ? parentComponent->GetChildren() : targetObject->GetDirectlyAttachedWorldComponents();
	TUsePointer<WorldComponent>* existing = componentsToSearchIn.FindIf([&componentName](TUsePointer<WorldComponent> comp) -> bool {
		return componentName == comp->GetComponentName();
	});

	TUsePointer<WorldComponent> target;
	if (existing != componentsToSearchIn.End()) {
		// A component the object built for itself in OnSetupComponents. DeserializeWorldComponent
		// deliberately leaves those untouched (its property/transform block is commented out) and
		// only walks into their children — mirrored here, so switching PIE to cloning changes speed
		// and nothing else.
		target = *existing;
	} else {
		target = targetObject->AddComponent(source->GetClass(), componentName);
		if (!target) return;
		CopyReflectedProperties(source->GetClass(), source.GetRaw(), target.GetRaw());
		target->SetRelativeLocation(source->GetRelativeLocation());
		target->SetRelativeRotation(source->GetRelativeRotation());
		target->SetRelativeScale(source->GetRelativeScale());
	}

	for (const auto& child : source->GetChildren()) {
		CloneWorldComponent(child, target, targetObject);
	}
}

Plu::DeserializationContext Plu::SceneManager::MakeDeserializationContext()
{
	DeserializationContext dc;
	dc.assetManager = mAssetManager;
	dc.scenesManager = mObjectManager->GetObjectAsUser<SceneManager>(*this->GetEngineObjectHandle());
	dc.shaderManager = mShaderManager;
	return dc;
}

void Plu::SceneManager::LoadGameObjectFromJSON(TUsePointer<SceneWorld> sceneWorld, JSON j)
{
	DeserializationContext dc = MakeDeserializationContext();
	LoadGameObjectFromJSON(&dc, sceneWorld, j);
}

void Plu::SceneManager::LoadGameObjectFromJSON(DeserializationContext* dc, TUsePointer<SceneWorld> sceneWorld, const JSON& j)
{
	PLU_PROFILE_SCOPE("LoadGameObjectFromJSON");
	if (!j.contains("typeName")) {
		PLU_ERROR("GameObject JSON without a typeName — skipping.");
		return;
	}
	// When the JSON carries a name, skip generating a default one: MakeDefaultObjectName scans the
	// whole scene (O(n^2) over a load) and the deserialization below overwrites the result anyway.
	// Scenes saved before mObjectName existed have no such field and still get the default name.
	bool hasSavedName = false;
	ReadStringFieldFromJson(j, "mObjectName", &hasSavedName);
	TypeInfo* objectClass = TypeRegistry::GetInstance()->GetTypeOfName(j["typeName"].get<std::string>().c_str());
	TUsePointer<GameObject> gameObject = hasSavedName
		? sceneWorld->SpawnGameObjectUnnamed(objectClass)
		: sceneWorld->SpawnGameObject(objectClass);
	if (!gameObject) {
		PLU_ERROR("No GameObject class of name {}! Maybe some python scripts were not run!", j["typeName"].get<std::string>().c_str());
		return;
	}
	TypeSerializer<TypeInfo*>::Deserialize(dc, j, gameObject->GetClass(), gameObject.GetRaw());
	// j is a const reference now, so a missing key can no longer be silently materialised as null
	// the way it was when this took the JSON by value — every optional key is checked.
	if (j.contains("location")) {
		Vec3 loc;
		TypeSerializer<Vec3>::Deserialize(dc, j["location"], &loc);
		gameObject->SetObjectLocation(loc);
	}
	if (j.contains("rotation")) {
		Vec3 rot;
		TypeSerializer<Vec3>::Deserialize(dc, j["rotation"], &rot);
		gameObject->SetObjectRotation(rot);
	}
	if (j.contains("scale")) {
		Vec3 scl;
		TypeSerializer<Vec3>::Deserialize(dc, j["scale"], &scl);
		gameObject->SetObjectScale(scl);
	}
	if (j.contains("worldComponents")) {
		for (const auto& worldComp : j["worldComponents"]) {
			DeserializeWorldComponent(dc, worldComp, nullptr, gameObject);
		}
	}

	if (!j.contains("components")) return;
	for (const auto& comp : j["components"]) {
		// The name used to be obtained by constructing a scratch component, deserializing the whole
		// JSON into it and destroying it again — for every component in the scene. It is a plain
		// reflected field, so read it out of the JSON directly.
		const String componentName = ReadStringFieldFromJson(comp, "mComponentName");
		TOwningPointer<GameObjectComponent>* findComp = gameObject->GetObjectComponents()->FindIf([&componentName](TOwningPointer<GameObjectComponent> find)->bool {
			return find->GetComponentName() == componentName;
		});
		if (findComp != gameObject->GetObjectComponents()->End()) {
			TOwningPointer<WorldComponent> compToPopulate = *findComp;
			TypeSerializer<TypeInfo*>::Deserialize(dc, comp, compToPopulate->GetClass(), compToPopulate.GetRaw());
			continue;
		}
		if (!comp.contains("typeName")) continue;
		TUsePointer<GameObjectComponent> component = gameObject->AddComponent(TypeRegistry::GetInstance()->GetTypeOfName(comp["typeName"].get<std::string>().c_str()), "comp");
		if (!component) continue;
		TypeSerializer<TypeInfo*>::Deserialize(dc, comp, component->GetClass(), component.GetRaw());
	}
}