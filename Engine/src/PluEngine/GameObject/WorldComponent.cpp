//
// Created by Plutex on 1/18/26.
//

#include "PluEngine/GameObject/WorldComponent.h"

#include "PluEngine/GameObject/GameObject.h"

Vec3 Plu::WorldComponent::GetWorldLocation()
{
	return GetParentGameObject()->GetObjectLocation();
}

Vec3 Plu::WorldComponent::GetWorldRotation()
{
	return GetParentGameObject()->GetObjectRotation();
}

Vec3 Plu::WorldComponent::GetWorldScale()
{
	return GetParentGameObject()->GetObjectScale();
}
