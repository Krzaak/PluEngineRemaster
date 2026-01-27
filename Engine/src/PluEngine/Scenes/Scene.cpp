//
// Created by Plutex on 1/18/26.
//

#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/GameObject/GameObjectComponent.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Renderer/Renderer.h"
#include "PluEngine/Renderer/RenderingInterfaces.h"

namespace Plu
{
	SceneWorld::~SceneWorld()
	{
		for (std::pair object: mGameObjects) {
			mEngineObjectManager->DestroyObject(*object.second->GetEngineObjectHandle());
			object.second = nullptr;
		}
	}

	void SceneWorld::Init(const TUsePointer<EngineObjectManager> &engineObjectManager, const TUsePointer<Renderer>& renderer)
	{
		mEngineObjectManager = engineObjectManager;
		mRenderer = renderer;
	}

	void SceneWorld::LoadGameObjects()
	{
	}

	void SceneWorld::UnloadGameObjects()
	{
		PLU_CORE_WARN("Unloading Game Objects (Shutdown)");
		for (auto gObj : mGameObjects) {
			gObj.second->OnEndPlay();
		}
		for (auto gObj : mGameObjects) {
			mEngineObjectManager->DestroyObject(*gObj.second->GetEngineObjectHandle());
		}
		mGameObjects.Clear();
	}

	void SceneWorld::Play()
	{
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
		mGameObjects.Insert(PluUUID(), newObject);
		newObject->InitGameObject(mEngineObjectManager->GetObjectAsUser<SceneWorld>(*GetEngineObjectHandle()), mEngineObjectManager);
		return newObject;
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
}
