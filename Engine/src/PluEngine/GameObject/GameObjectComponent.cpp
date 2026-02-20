//
// Created by Plutex on 1/18/26.
//

#include "PluEngine/GameObject/GameObjectComponent.h"

namespace Plu
{
	void GameObjectComponent::SetParentGameObject(TUsePointer<GameObject> newParent)
	{
		mParentObject = newParent;
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
}
