//
// Created by Plutex on 1/12/26.
//

#include "EditorScene.h"

#include "PluEngine/GameObject/GameObject.h"
#include "PluEngine/Objects/EngineObjectManager.h"

namespace Plu
{
	EditorScene::EditorScene()
	{
	}

	EditorScene::~EditorScene()
	{
		for (std::pair object: mGameObjects) {
			mEngineObjectManager->DestroyObject(*object.second->GetEngineObjectHandle());
			object.second = nullptr;
		}
	}

	void EditorScene::Init(const TUsePointer<EngineObjectManager> &engineObjectManager)
	{
		mEngineObjectManager = engineObjectManager;
	}

	DynamicArray<TUsePointer<GameObject>> EditorScene::GetAllGameObjects()
	{
		DynamicArray<TUsePointer<GameObject>> result;
		result.Reserve(mGameObjects.Size());
		for (std::pair obj : mGameObjects) {
			result.PushBack(obj.second);
		}
		return result;
	}
}
