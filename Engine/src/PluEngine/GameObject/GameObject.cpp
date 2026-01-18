//
// Created by Plutex on 1/18/26.
//

#include "PluEngine/GameObject/GameObject.h"

Vec3 Plu::GameObject::GetObjectLocation() const
{
	return mLocation;
}

Vec3 Plu::GameObject::GetObjectRotation() const
{
	return mRotation;
}

Vec3 Plu::GameObject::GetObjectScale() const
{
	return mScale;
}

void Plu::GameObject::SetObjectLocation(const Vec3 &location)
{
	mLocation = location;
}

void Plu::GameObject::SetObjectRotation(const Vec3 &rotation)
{
	mRotation = rotation;
}

void Plu::GameObject::SetObjectScale(const Vec3 &scale)
{
	mScale = scale;
}
