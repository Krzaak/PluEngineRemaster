//
// Created by Plutex on 1/18/26.
//

#include "PluEngine/BasicEngineClasses/Components/PhysicsBodyComponent.h"
#include "PluEngine/BasicEngineClasses/GameObjects/SpectatorPuppet.h"
#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/GameObject/GameObjectComponent.h"
#include "PluEngine/GameObject/WorldComponent.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"

namespace Plu
{
	SceneWorld::~SceneWorld()
	{
	}

	void SceneWorld::Init(const TUsePointer<EngineObjectManager> &engineObjectManager, const TUsePointer<Renderer>& renderer, const TUsePointer<GameClient>& client)
	{
		mEngineObjectManager = engineObjectManager;
		mRenderer = renderer;
		mClient = client;
		EngineObjectHandle physicsWorldUser = mEngineObjectManager->CreateObject<PhysicsWorld>();
		mPhysicsWorld = mEngineObjectManager->GetObjectAsOwner<PhysicsWorld>(physicsWorldUser);
		mPhysicsWorld->Init(mEngineObjectManager->GetObjectAsUser<SceneWorld>(*GetEngineObjectHandle()), mEngineObjectManager);
	}

	void SceneWorld::LoadGameObjects()
	{
	}

	void SceneWorld::UnloadGameObjects()
	{
		PLU_CORE_WARN("Unloading Game Objects (Shutdown) - Scene: {}", Info ? Info->URL.CStr() : GetDisplayName().CStr());
		for (const auto& gObj : mGameObjects) {
			mGameObjects[gObj.first]->OnEndPlay();
		}
		mGameMode = nullptr;
		mPhysicsWorld->Shutdown();
		mControllers.Clear();
		mObjectsToDestroy.Clear();
		mObjectsToBegin.Clear();
		for (const auto& gObj : mGameObjects) {
			DeleteGameObject(*gObj.second->GetEngineObjectHandle(), false);
		}
		HandleDestroy();
		mEngineObjectManager->DestroyObject(*mPhysicsWorld->GetEngineObjectHandle());
		mPhysicsWorld = nullptr;
		mGameObjects.Clear();
	}

	void SceneWorld::Play()
	{
		mGameMode = SpawnGameObject(GameModeClass.GetRawType());
		HandleBeginPlay();
		mPhysicsWorld->Play();
		mIsPlaying = true;
	}

	void SceneWorld::HandleBeginPlay()
	{
		for (const auto& obj : mObjectsToBegin) {
			obj->OnBeginPlay();
			for (auto worldComp : obj->mWorldComponents) {
				worldComp->OnBeginPlay();
			}
			for (auto comp : obj->mComponents) {
				comp->OnBeginPlay();
			}
		}
		mObjectsToBegin.Clear();
	}

	void SceneWorld::HandleDestroy()
	{
		bool destroyedSmth = false;
		for (const auto& obj : mObjectsToDestroy) {
			destroyedSmth = true;
			TUsePointer<GameObject> object = obj.first;
			if (obj.second) object->OnEndPlay();
			for (const auto& wc : *object->GetObjectWorldComponents()) {
				IRenderable* rendrPtr = dynamic_cast<IRenderable *>(wc.GetRaw());
				if (rendrPtr) {
					mRenderer->RemoveRenderable(rendrPtr);
				}
				if (IRendererCamera* camera = dynamic_cast<IRendererCamera *>(wc.GetRaw())) {
					if (camera == mRenderer->GetCamera()) {
						mRenderer->SetCamera(nullptr);
					}
				}
			}
			object->Cleanup();
			if (mGameObjects.Contains(object->GetObjectUUID())) {
				mGameObjects.Remove(object->mUuid);
			}
			mEngineObjectManager->DestroyObject(*object->GetEngineObjectHandle());
		}
		mObjectsToDestroy.Clear();
		if (destroyedSmth) {
			mRenderer->ClearRenderables();
			this->LoadRenderables();
		}
	}

	TUsePointer<Controller> SceneWorld::GetControllerByID(UInt16 playerID)
	{
		return mControllers.Contains(playerID) ? mControllers[playerID] : nullptr;
	}

	void SceneWorld::TickScene(float deltaTime)
	{
		mPhysicsWorld->Update(deltaTime);
		for (const auto& gameObject : mGameObjects) {
			gameObject.second->TickObject(deltaTime);
		}
		HandleDestroy();
		HandleBeginPlay();
	}

	void SceneWorld::LoadRenderables()
	{
		for (const auto& gameobject : mGameObjects) {
			for (const auto& worldComp : gameobject.second->mWorldComponents) {
				GameObjectComponent* compPtr = worldComp.GetRaw();
				IRenderable* rendrPtr = dynamic_cast<IRenderable *>(compPtr);
				if (rendrPtr) {
					mRenderer->AddRenderable(rendrPtr);
				}
			}
		}
	}

	void SceneWorld::NewGameObjectComponent(const TOwningPointer<GameObjectComponent>& component)
	{
		component->OnSetupComponent();
		if (component->GetClass()->IsDerivedOfOrSame(PhysicsBodyComponent::GetStaticClass())) {
			mPhysicsWorld->NewPhysicsComponent(component, mIsPlaying);
		}
		GameObjectComponent* compPtr = component.GetRaw();
		IRenderable* rendrPtr = dynamic_cast<IRenderable *>(compPtr);
		if (rendrPtr) {
			mRenderer->AddRenderable(rendrPtr);
		}
	}

	TUsePointer<GameObject> SceneWorld::SpawnGameObject(TClassPointer<GameObject> objectClass)
	{
		if (!objectClass) {
			PLU_CORE_ERROR("Invalid Class for spawning GameObject!");
			return nullptr;
		}
		TUsePointer<GameObject> newObjectUser = mEngineObjectManager->CreateObject(objectClass);
		TOwningPointer<GameObject> newObject = mEngineObjectManager->GetObjectAsOwner<GameObject>(newObjectUser->GetObjectHandle());
		PluUUID uuid;
		mGameObjects.Insert(uuid, newObject);
		newObject->mUuid = uuid;
		newObject->InitGameObject(mEngineObjectManager->GetObjectAsUser<SceneWorld>(*GetEngineObjectHandle()), mEngineObjectManager);
		try {
			newObject->OnSetupComponents();
		} catch (pybind11::error_already_set& e) {
			PLU_CORE_ERROR("Error has happened on SetupComponents phase on object {}, what -> {}", newObject->GetDisplayName().CStr(), e.what());
		}
		mObjectsToBegin.PushBack(newObject);
		return newObject;
	}

	void SceneWorld::DeleteGameObject(EngineObjectHandle gameObject, bool callEndPlay)
	{
		TOwningPointer<GameObject> object = mEngineObjectManager->GetObjectAsOwner<GameObject>(gameObject);
		if (!object) return;
		mObjectsToDestroy.PushBack({object, callEndPlay});
	}

	DynamicArray<TUsePointer<GameObject>> SceneWorld::GetAllGameObjects()
	{
		DynamicArray<TUsePointer<GameObject>> result;
		result.Reserve(mGameObjects.Size());
		for (const std::pair<UInt64, TOwningPointer<GameObject>>& obj : mGameObjects) {
			result.PushBack(obj.second);
		}
		return result;
	}

	void SceneWorld::GetFormattedGameObjectNames(DynamicArray<String>* result)
	{
		result->Clear();
		result->Reserve(mGameObjects.Size());
		for (auto obj : mGameObjects) {
			result->PushBack(obj.second->GetDisplayName());
		}
	}

	TUsePointer<GameObject> SceneWorld::GetGameObjectByUUID(PluUUID uuid)
	{
		return mGameObjects[uuid];
	}

	TUsePointer<GameObject> SceneWorld::GetGameObjectOfClass(TClassPointer<GameObject> gameObjectClass)
	{
		for (const auto& gameObject : mGameObjects) {
			if (gameObject.second->GetClass()->IsDerivedOfOrSame(gameObjectClass)) {
				return gameObject.second;
			}
		}
		return nullptr;
	}

	DynamicArray<TUsePointer<GameObject>> SceneWorld::GetAllGameObjectsOfClass(
		TClassPointer<GameObject> gameObjectClass)
	{
		DynamicArray<TUsePointer<GameObject>> gameObjects;
		for (const auto& gameObject : mGameObjects) {
			if (gameObject.second->GetClass()->IsDerivedOfOrSame(gameObjectClass)) {
				gameObjects.PushBack(gameObject.second);
			}
		}
		return gameObjects;
	}

	void SceneWorld::JoinPlayerLocally(UInt16 playerID)
	{
		TUsePointer<Controller> controller = SpawnGameObject(mGameMode->ControllerClass ? mGameMode->ControllerClass : TClassPointer<Controller>(Controller::GetStaticClass()));
		TUsePointer<Puppet> puppet = SpawnGameObject(mGameMode->PuppetClass ? mGameMode->PuppetClass : TClassPointer<Puppet>(SpectatorPuppet::GetStaticClass()));
		mControllers.Insert(playerID, controller);
		controller->mPlayerID = playerID;
		controller->Possess(puppet);
	}

	PhysicsWorld * SceneWorld::GetPhysicsWorld()
	{
		return mPhysicsWorld.GetRaw();
	}

	TUsePointer<IScenesManager> gScenesManager;

	void IScenesManager::InitSceneManagerForPython(TUsePointer<IScenesManager> scenesManager)
	{
		gScenesManager = scenesManager;
	}

}
