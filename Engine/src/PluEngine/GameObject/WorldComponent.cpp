//
// Created by Plutex on 1/18/26.
//

#include "PluEngine/GameObject/WorldComponent.h"

#include "PluEngine/GameObject/GameObject.h"

void Plu::WorldComponent::Cleanup()
{
	for (const auto& component : mWorldComponents) {
		component->Cleanup();
		GetObjectManagerFromParent()->DestroyObject(*component->GetEngineObjectHandle());
	}
	mWorldComponents.Clear();
}

Plu::TUsePointer<Plu::GameObjectComponent> Plu::WorldComponent::GetParentComponent() const
{
	return mParentComponent;
}

DynamicArray<Plu::TUsePointer<Plu::WorldComponent>> Plu::WorldComponent::GetChildren()
{
	DynamicArray<TUsePointer<WorldComponent>> children(mWorldComponents.Size());
	for (const auto& child : mWorldComponents) {
		children.PushBack(child);
	}
	return children;
}

void Plu::WorldComponent::AttachTo(GameObjectComponent *newAttachPoint)
{
	if (!mParentComponent) {
		GetParentGameObject()->OnDetachComponent(ThisAsOwner());
	}
	if (!newAttachPoint) {
		mParentComponent = nullptr;
		GetParentGameObject()->OnAttachComponent(ThisAsOwner(), nullptr);
		return;
	}
	mParentComponent = GetObjectManagerFromParent()->GetObjectAsUser<GameObjectComponent>(*newAttachPoint->GetEngineObjectHandle());
	GetParentGameObject()->OnAttachComponent(ThisAsOwner(), mParentComponent);
}

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
