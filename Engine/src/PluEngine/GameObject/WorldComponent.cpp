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

void Plu::WorldComponent::SetWorldLocation(const Vec3 newLoc)
{
	GetParentGameObject()->SetObjectLocation(newLoc);
}

void Plu::WorldComponent::SetWorldRotation(const Vec3 newRot)
{
	GetParentGameObject()->SetObjectRotation(newRot);
}

void Plu::WorldComponent::SetWorldScale(const Vec3 newScale)
{
	GetParentGameObject()->SetObjectScale(newScale);
}
