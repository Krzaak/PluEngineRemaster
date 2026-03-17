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

	TUsePointer<GameObject> GameObjectComponent::GetParentGameObject() const
	{
		return mParentObject;
	}

	TUsePointer<GameObjectComponent> GameObjectComponent::GetParentComponent() const
	{
		return mParentComponent;
	}

	void GameObjectComponent::AttachTo(GameObjectComponent *newAttachPoint)
	{
		if (!newAttachPoint) {
			mParentComponent = nullptr;
			return;
		}
		mParentComponent = GetObjectManagerFromParent()->GetObjectAsUser<GameObjectComponent>(*newAttachPoint->GetEngineObjectHandle());
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
