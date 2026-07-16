//
// Created by Plutex on 5/29/26.
//

#include "PluEngine/Scenes/SceneWorld.h"
#include "PluEngine/BasicEngineClasses/Components/PhysicsBodyComponent.h"
#include "PluEngine/BasicEngineClasses/Components/StaticMeshComponent.h"
#include "PluEngine/BasicEngineClasses/Components/SkeletalMeshComponent.h"
#include "PluEngine/BasicEngineClasses/GameObjects/SpectatorPuppet.h"
#include "PluEngine/BasicEngineClasses/GameObjects/Lights/DirectionalLight.h"
#include "PluEngine/GameCore/GameClient.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/GameObject/GameObjectComponent.h"
#include "PluEngine/GameObject/WorldComponent.h"
#include "PluEngine/Input/InputManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Physics/PhysicsWorld.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"

namespace Plu
{
	SceneWorld::~SceneWorld()
	{
	}

	void SceneWorld::Init(const TUsePointer<EngineObjectManager> &engineObjectManager, const TUsePointer<GameClient>& client)
	{
		mEngineObjectManager = engineObjectManager;
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
		mPendingSpawns.Clear();
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
		// Objects spawned during play need their physics body before OnBeginPlay runs, since
		// begin-play code may already read velocity / move the body. Before Play() the whole
		// batch is built by PhysicsWorld::Play() instead.
		if (mIsPlaying) mPhysicsWorld->FlushPendingBodies();
		// Move the batch aside first: OnBeginPlay may spawn objects, which pushes into
		// mObjectsToBegin and would reallocate the array we are iterating. Those objects begin
		// play on the next pass.
		DynamicArray<TUsePointer<GameObject>> batch = std::move(mObjectsToBegin);
		mObjectsToBegin.Clear();
		for (const auto& obj : batch) {
			if (!obj) continue;
			obj->OnBeginPlay();
			for (auto worldComp : obj->mWorldComponents) {
				worldComp->OnBeginPlay();
			}
			for (auto comp : obj->mComponents) {
				comp->OnBeginPlay();
			}
			// Directional light is now registered at spawn time (see SpawnGameObject), so the
			// editor preview gets lighting/shadows without play. Nothing to do here.
		}
	}

	void SceneWorld::HandleDestroy()
	{
		bool destroyedSmth = false;
		// Same reason as in HandleBeginPlay: OnEndPlay may destroy further objects, which pushes
		// into mObjectsToDestroy mid-iteration. Those are handled on the next pass.
		DynamicArray<std::pair<TUsePointer<GameObject>, bool>> batch = std::move(mObjectsToDestroy);
		mObjectsToDestroy.Clear();
		for (const auto& obj : batch) {
			if (!obj.first) continue;
			if (!mGameObjects.Contains(obj.first->GetObjectUUID())) {
				PLU_CORE_ERROR("Destroying Invalid GameObject!");
				continue;
			}
			destroyedSmth = true;
			TUsePointer<GameObject> object = obj.first;
			if (obj.second) object->OnEndPlay();
			if (mStaticMeshRenderables.Contains(object->GetObjectUUID())) {
				mStaticMeshRenderables.Remove(object->GetObjectUUID());
			}
			if (mSkeletalMeshRenderables.Contains(object->GetObjectUUID())) {
				mSkeletalMeshRenderables.Remove(object->GetObjectUUID());
			}
			if (object == mDirectionalLight) {
				mDirectionalLight = nullptr;
			}
			mPhysicsWorld->RemoveGameObjectBodies(object.GetRaw());
			object->Cleanup();
			mGameObjects.Remove(object->mUuid);
			mEngineObjectManager->DestroyObject(*object->GetEngineObjectHandle());
		}
#ifdef PLU_ENGINE_EDITOR_BUILD
		if (destroyedSmth) {
			PLU_CORE_WARN("Destroyed {} objects!", batch.Size());
			GetObjectEventDispatcher()->Dispatch("GameObjectsChanged", nullptr);
		}
#endif
	}

	void SceneWorld::FlushPendingSpawns()
	{
		if (mPendingSpawns.IsEmpty()) return;
		for (const auto& object : mPendingSpawns) {
			mGameObjects.Insert(object->mUuid, object);
		}
		mPendingSpawns.Clear();
	}

	TUsePointer<Controller> SceneWorld::GetControllerByID(UInt16 playerID)
	{
		return mControllers.Contains(playerID) ? mControllers[playerID] : nullptr;
	}

	void SceneWorld::TickScene(float deltaTime)
	{
		// Picks up bodies for components added outside of a spawn (e.g. AddComponent from a tick).
		mPhysicsWorld->FlushPendingBodies();
		PLU_TIMER_START("Physics Update");
		mPhysicsWorld->Update(deltaTime);
		PLU_TIMER_END("Physics Update");
		PLU_TIMER_START("GameObjects Ticks");
		// A tick may spawn objects; while this flag is up SpawnGameObject parks them in
		// mPendingSpawns instead of inserting into the map we are iterating. They join the map
		// right after the loop and tick from the next frame on.
		mTickingGameObjects = true;
		for (const auto& gameObject : mGameObjects) {
			gameObject.second->TickObject(deltaTime);
		}
		mTickingGameObjects = false;
		FlushPendingSpawns();
		PLU_TIMER_END("GameObjects Ticks");
		HandleDestroy();
		HandleBeginPlay();
	}

	void SceneWorld::NewGameObjectComponent(const TOwningPointer<GameObjectComponent>& component)
	{
		component->OnSetupComponent();
		if (component->GetClass()->IsDerivedOfOrSame(PhysicsBodyComponent::GetStaticClass())) {
			mPhysicsWorld->NewPhysicsComponent(component);
		}
		if (component->GetClass()->IsDerivedOfOrSame(StaticMeshComponent::GetStaticClass())) {
			if (mStaticMeshRenderables.Contains(component->GetParentGameObject()->GetObjectUUID())) {
				mStaticMeshRenderables[component->GetParentGameObject()->GetObjectUUID()].PushBack(component);
			} else {
				mStaticMeshRenderables[component->GetParentGameObject()->GetObjectUUID()] = {component};
			}
		}
		if (component->GetClass()->IsDerivedOfOrSame(SkeletalMeshComponent::GetStaticClass())) {
			if (mSkeletalMeshRenderables.Contains(component->GetParentGameObject()->GetObjectUUID())) {
				mSkeletalMeshRenderables[component->GetParentGameObject()->GetObjectUUID()].PushBack(component);
			} else {
				mSkeletalMeshRenderables[component->GetParentGameObject()->GetObjectUUID()] = {component};
			}
		}
	}

	void SceneWorld::OnGameObjectScaleChanged(GameObject* gameObject)
	{
		if (!mIsPlaying || !gameObject) return;
		// Only objects that already have a physics body need their colliders rebuilt.
		if (!mEngineObjectManager->IsValid(gameObject->mPhysicsBodyHandle)) return;
		mPhysicsWorld->RebuildGameObjectBody(gameObject);
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
		newObject->mUuid = uuid;
		if (mTickingGameObjects) {
			mPendingSpawns.PushBack(newObject);
		} else {
			mGameObjects.Insert(uuid, newObject);
		}
		newObject->InitGameObject(mEngineObjectManager->GetObjectAsUser<SceneWorld>(*GetEngineObjectHandle()), mEngineObjectManager);
		try {
			newObject->OnSetupComponents();
		} catch (pybind11::error_already_set& e) {
			PLU_CORE_ERROR("Error has happened on SetupComponents phase on object {}, what -> {}", newObject->GetDisplayName().CStr(), e.what());
		}
		// Register the directional light at spawn time (independent of play state) so the editor
		// preview renders lighting/shadows without entering PIE. Mirrors how StaticMeshComponents
		// register in NewGameObjectComponent during OnSetupComponents. HandleBeginPlay no longer
		// owns this responsibility.
		if (newObject->GetClass()->IsDerivedOfOrSame(DirectionalLight::GetStaticClass())) {
			PLU_CORE_ASSERT(!mDirectionalLight, "There can be only one Directional Light in a scene")
			mDirectionalLight = mEngineObjectManager->GetObjectAsOwner<DirectionalLight>(newObject->GetObjectHandle());
		}
		mObjectsToBegin.PushBack(newObject);
		mNewGameObjectSpawned = true;
		GetObjectEventDispatcher()->Dispatch("GameObjectsChanged", nullptr);
		return newObject;
	}

	void SceneWorld::DeleteGameObject(EngineObjectHandle gameObject, bool callEndPlay)
	{
		TOwningPointer<GameObject> object = mEngineObjectManager->GetObjectAsOwner<GameObject>(gameObject);
		if (!object) return;
		mObjectsToDestroy.PushBack({object, callEndPlay});
	}

	void SceneWorld::DestroyGameObject(GameObject *gameObject)
	{
		DeleteGameObject(gameObject->GetObjectHandle());
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
		auto* found = mGameObjects.Find(uuid);
		if (found) {
			return *found;
		}
		if (mPendingSpawns.IsEmpty()) {
			return nullptr;
		}
		mPendingSpawns.FindIf([&](TOwningPointer<GameObject> gameObject) -> bool {
			return gameObject->GetObjectUUID() == uuid;
		});
		return nullptr;
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
		if (!mGameObjectsPerClassCache.Contains(gameObjectClass.GetRawType()->TypeName) || mNewGameObjectSpawned) {
			DynamicArray<TUsePointer<GameObject>> gameObjects;
			for (const auto& gameObject : mGameObjects) {
				if (gameObject.second->GetClass()->IsDerivedOfOrSame(gameObjectClass)) {
					gameObjects.PushBack(gameObject.second);
				}
			}
			if (mGameObjectsPerClassCache.Contains(gameObjectClass.GetRawType()->TypeName)) {
				mGameObjectsPerClassCache.Remove(gameObjectClass.GetRawType()->TypeName);
			}
			mGameObjectsPerClassCache.Insert(gameObjectClass.GetRawType()->TypeName, gameObjects);
			mNewGameObjectSpawned = false;
		}
		return mGameObjectsPerClassCache[gameObjectClass.GetRawType()->TypeName];
	}

	void SceneWorld::JoinPlayerLocally(UInt16 playerID)
	{
		PLU_CORE_ASSERT(mGameMode, "No game mode!");
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

}
