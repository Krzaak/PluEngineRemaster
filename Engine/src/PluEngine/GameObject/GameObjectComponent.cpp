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

	TUsePointer<GameObject> GameObjectComponent::GetParentGameObject()
	{
		return mParentObject;
	}
}
