//
// Created by Plutex on 1/18/26.
//

#include "PluEngine/GameObject/GameObjectComponent.h"

#include "PluEngine/GameObject/GameObject.h"

namespace Plu
{
	void GameObjectComponent::SetParentGameObject(TUsePointer<GameObject> newParent)
	{
		mParentObject = newParent;
	}

	TUsePointer<EngineObjectManager> GameObjectComponent::GetObjectManagerFromParent()
	{
		return GetParentGameObject()->mObjectManager;
	}

	TUsePointer<SceneWorld> GameObjectComponent::GetWorld()
	{
		return GetParentGameObject()->mWorld;
	}

	bool GameObjectComponent::IsActivated() const
	{
		return mIsActivated;
	}

	void GameObjectComponent::Activate()
	{
		mIsActivated = true;
	}

	void GameObjectComponent::Deactivate()
	{
		mIsActivated = false;
	}

	TUsePointer<GameObject> GameObjectComponent::GetParentGameObject()
	{
		return mParentObject;
	}

	String GameObjectComponent::GetComponentName()
	{
		return mComponentName;
	}

	void GameObjectComponent::SetComponentName(String name)
	{
		mComponentName = name;
	}
}
