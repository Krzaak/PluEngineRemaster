//
// Created by Plutex on 1/18/26.
//

#include "PluEngine/GameObject/GameObject.h"

#include "PluEngine/GameObject/GameObjectComponent.h"
#include "PluEngine/Objects/EngineObjectManager.h"

void Plu::GameObject::InitGameObject(const TUsePointer<class SceneWorld>& sceneWorld,
                                     const TUsePointer<class EngineObjectManager>& objectManager)
{
	mObjectManager = objectManager;
	mWorld = sceneWorld;
}

Plu::TUsePointer<Plu::GameObjectComponent> Plu::GameObject::AddComponent(TypeInfo *componentClass)
{
	PLU_CORE_ASSERT(componentClass->IsDerivedOfOrSame(GameObjectComponent::GetStaticClass()), "Tried to create new component with invalid Component Class! Possibly class is not derived from GameObjectComponent")
	TOwningPointer<GameObjectComponent> newComponent = mObjectManager->CreateObject(componentClass);
	mComponents.PushBack(newComponent);
	newComponent->SetParentGameObject(mObjectManager->GetObjectAsUser<GameObject>(*GetEngineObjectHandle()));
	return newComponent;
}

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
