//
// Created by Plutex on 1/18/26.
//

#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/GameObject/GameObjectComponent.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"

namespace Plu
{
	bool SceneWorld::IsKeyDown(Key key) const
	{
		return mClient->mInputManager->GetInputBackend()->GetKeyboard().IsDown(key);
	}

	SceneWorld::~SceneWorld()
	{
	}

	void SceneWorld::Init(const TUsePointer<EngineObjectManager> &engineObjectManager, const TUsePointer<Renderer>& renderer, const TUsePointer<GameClient>& client)
	{
		mEngineObjectManager = engineObjectManager;
		mRenderer = renderer;
		mClient = client;
	}

	void SceneWorld::LoadGameObjects()
	{
	}

	void SceneWorld::UnloadGameObjects()
	{
		PLU_CORE_WARN("Unloading Game Objects (Shutdown)");
		for (const auto& gObj : mGameObjects) {
			mGameObjects[gObj.first]->OnEndPlay();
		}
		GameHashMap<UInt64, TOwningPointer<GameObject>> copyGameObjects = mGameObjects;
		for (const auto& gObj : copyGameObjects) {
			DeleteGameObject(*gObj.second->GetEngineObjectHandle(), false);
		}
		mGameObjects.Clear();
	}

	void SceneWorld::Play()
	{
		mGameMode = SpawnGameObject(GameModeClass.GetRawType());
	}

	TUsePointer<Controller> SceneWorld::GetControllerByID(UInt16 playerID)
	{
		return mControllers.Contains(playerID) ? mControllers[playerID] : nullptr;
	}

	void SceneWorld::TickScene(float deltaTime)
	{
		for (const auto& gameObject : mGameObjects) {
			gameObject.second->OnUpdate(deltaTime);
			for (const auto& worldComp : gameObject.second->mWorldComponents) {
				worldComp->OnUpdate(deltaTime);
			}
			for (const auto& comp : gameObject.second->mComponents) {
				comp->OnUpdate(deltaTime);
			}
		}
	}

	void SceneWorld::LoadRenderables()
	{
		for (const auto& gameobject : mGameObjects) {
			for (const auto& worldComp : gameobject.second->mWorldComponents) {
				GameObjectComponent* compPtr = worldComp.GetRaw();
				IRenderable* rendrPtr = dynamic_cast<IRenderable *>(compPtr);
				if (rendrPtr) {
					PLU_CORE_INFO("New component implements IRenderable");
					mRenderer->AddRenderable(rendrPtr);
				}
			}
		}
	}

	void SceneWorld::NewGameObjectComponent(const TOwningPointer<GameObjectComponent>& component)
	{
		GameObjectComponent* compPtr = component.GetRaw();
		IRenderable* rendrPtr = dynamic_cast<IRenderable *>(compPtr);
		if (rendrPtr) {
			PLU_CORE_INFO("New component implements IRenderable");
			mRenderer->AddRenderable(rendrPtr);
		}
	}

	TUsePointer<GameObject> SceneWorld::SpawnGameObject(TypeInfo *objectClass)
	{
		TOwningPointer<GameObject> newObject = mEngineObjectManager->CreateObject(objectClass);
		PluUUID uuid;
		mGameObjects.Insert(uuid, newObject);
		newObject->mUuid = uuid;
		newObject->InitGameObject(mEngineObjectManager->GetObjectAsUser<SceneWorld>(*GetEngineObjectHandle()), mEngineObjectManager);
		return newObject;
	}

	void SceneWorld::DeleteGameObject(EngineObjectHandle gameObject, bool callEndPlay)
	{
		TOwningPointer<GameObject> object = mEngineObjectManager->GetObjectAsOwner<GameObject>(gameObject);
		if (!object) return;
		if (callEndPlay) object->OnEndPlay();
		for (auto wc : object->mWorldComponents) {
			IRenderable* rendrPtr = dynamic_cast<IRenderable *>(wc.GetRaw());
			if (rendrPtr) {
				PLU_CORE_INFO("Removing IRenderable");
				mRenderer->RemoveRenderable(rendrPtr);
			}
		}
		object->Cleanup();
		if (mGameObjects.Contains(object->GetObjectUUID())) {
			PLU_CORE_INFO("Removing Object");
			mGameObjects.Remove(object->mUuid);
		}
		mEngineObjectManager->DestroyObject(gameObject);
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

	void SceneWorld::JoinPlayerLocally(UInt16 playerID)
	{
		TUsePointer<Puppet> puppet = SpawnGameObject(mGameMode->PuppetClass);
		TUsePointer<Controller> controller = SpawnGameObject(mGameMode->ControllerClass);
		mControllers.Insert(playerID, controller);
		controller->mPlayerID = playerID;
		controller->Possess(puppet);
	}
}
