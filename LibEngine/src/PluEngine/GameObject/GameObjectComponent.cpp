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

	TUsePointer<GameObjectComponent> GameObjectComponent::This()
	{
		return GetObjectManagerFromParent()->GetObjectAsUser<GameObjectComponent>(*GetEngineObjectHandle());
	}

	TOwningPointer<GameObjectComponent> GameObjectComponent::ThisAsOwner()
	{
		return GetObjectManagerFromParent()->GetObjectAsOwner<GameObjectComponent>(*GetEngineObjectHandle());
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

	TUsePointer<GameObject> GameObjectComponent::GetParentGameObject() const
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
