//
// Created by Plutex on 1/18/26.
//

#include "PluEngine/GameObject/GameObject.h"

#include "PluEngine/PluUtils.h"
#include "PluEngine/GameObject/GameObjectComponent.h"
#include "PluEngine/GameObject/WorldComponent.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"

void Plu::GameObject::InitGameObject(const TUsePointer<class SceneWorld> &sceneWorld,
	const TUsePointer<class EngineObjectManager> &objectManager)
{
	mObjectManager = objectManager;
	mWorld = sceneWorld;
}

void Plu::GameObject::OnAttachComponent(const TOwningPointer<WorldComponent>& component,
	const TUsePointer<WorldComponent>& attachPoint)
{
	mRedoWorldComponentList = true;
	if (!attachPoint) {
		if (!mWorldComponents.Contains(component)) {
			mWorldComponents.PushBack(component);
		}
	} else {
		if (!attachPoint->mWorldComponents.Contains(component)) {
			attachPoint->mWorldComponents.PushBack(component);
		}
	}
}

void Plu::GameObject::OnDetachComponent(const TOwningPointer<WorldComponent>& component)
{
	mWorldComponents.Remove(component);
}

Plu::TUsePointer<Plu::GameObject> Plu::GameObject::This()
{
	return mObjectManager->GetObjectAsUser<GameObject>(*GetEngineObjectHandle());
}

Plu::TUsePointer<Plu::SceneWorld> Plu::GameObject::GetWorld()
{
	return mWorld;
}

Plu::GameObject::~GameObject()
{
}

void Plu::GameObject::Cleanup()
{
	for (auto component : mComponents) {
		mObjectManager->DestroyObject(*component->GetEngineObjectHandle());
	}
	for (const auto& component : mWorldComponents) {
		component->Cleanup();
		mObjectManager->DestroyObject(*component->GetEngineObjectHandle());
	}
	mComponents.Clear();
	mWorldComponents.Clear();
}

void Plu::GameObject::TickObject(float deltaTime)
{
	if (GetInputHandler()) {
		GetInputHandler()->TickHandler();
	}
	OnUpdate(deltaTime);
	for (const auto& worldComp : mWorldComponents) {
		worldComp->OnUpdate(deltaTime);
	}
	for (const auto& comp : mComponents) {
		comp->OnUpdate(deltaTime);
	}
}

Plu::TUsePointer<Plu::GameObjectComponent> Plu::GameObject::AddComponent(TClassPointer<GameObjectComponent> componentClass, String componentName)
{
	if (!componentClass.GetRawType()) return nullptr;
	PLU_CORE_ASSERT(componentClass.GetRawType()->IsDerivedOfOrSame(GameObjectComponent::GetStaticClass()), "Tried to create new component with invalid Component Class! Possibly class is not derived from GameObjectComponent")
	TOwningPointer<GameObjectComponent> newComponent = mObjectManager->CreateObject(componentClass);
	newComponent->mComponentName = componentName;
	RegisterComponent(newComponent);
	return newComponent;
}

void Plu::GameObject::RegisterComponent(TOwningPointer<GameObjectComponent> component)
{
	if (component->GetClass()->IsDerivedOfOrSame(WorldComponent::GetStaticClass())) {
		mWorldComponents.PushBack(component);
		mRedoWorldComponentList = true;
	} else {
		mComponents.PushBack(component);
	}
	component->SetParentGameObject(mObjectManager->GetObjectAsUser<GameObject>(*GetEngineObjectHandle()));
	mWorld->NewGameObjectComponent(component);
}

DynamicArray<Plu::TOwningPointer<Plu::GameObjectComponent>> * Plu::GameObject::GetObjectComponents()
{
	return &mComponents;
}

void GatherWorldComponentChildren(DynamicArray<Plu::TUsePointer<Plu::WorldComponent>>* components, Plu::TUsePointer<Plu::WorldComponent> component)
{
	components->Append(component->GetChildren());
	for (const auto& child : component->GetChildren()) {
		GatherWorldComponentChildren(components, child);
	}
}

DynamicArray<Plu::TUsePointer<Plu::WorldComponent>>* Plu::GameObject::GetObjectWorldComponents(bool force)
{
	if (mRedoWorldComponentList || force) {
		mCachedWorldComponents.Clear();
		for (const auto& worldComp : mWorldComponents) {
			mCachedWorldComponents.PushBack(worldComp);
			GatherWorldComponentChildren(&mCachedWorldComponents, worldComp);
		}
		mRedoWorldComponentList = false;
	}
	return &mCachedWorldComponents;
}

DynamicArray<Plu::TUsePointer<Plu::WorldComponent>> Plu::GameObject::GetDirectlyAttachedWorldComponents()
{
	DynamicArray<TUsePointer<WorldComponent>> components(mWorldComponents.Size());
	for (const auto& comp : mWorldComponents) {
		components.PushBack(comp);
	}
	return components;
}

Plu::TUsePointer<Plu::GameObjectComponent> Plu::GameObject::GetActivatedComponentByClass(
	const TClassPointer<GameObjectComponent>& componentClass)
{
	if (componentClass.GetRawType()->IsDerivedOfOrSame(WorldComponent::GetStaticClass())) {
		for (const auto& worldComp : mWorldComponents) {
			if (worldComp->GetClass()->IsDerivedOfOrSame(componentClass)) {
				return worldComp;
			}
		}
	} else {
		for (const auto& comp : mComponents) {
			if (comp->GetClass()->IsDerivedOfOrSame(componentClass)) {
				return comp;
			}
		}
	}
	return nullptr;
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
	GetObjectEventDispatcher()->Dispatch("LocationChange");
}

void Plu::GameObject::SetObjectRotation(const Vec3 &rotation)
{
	NormalizeVec3Rotation(const_cast<Vec3 *>(&rotation));
	GetObjectEventDispatcher()->Dispatch("RotationChange");
}

void Plu::GameObject::SetObjectScale(const Vec3 &scale)
{
	mScale = scale;
	GetObjectEventDispatcher()->Dispatch("ScaleChange");
}

Vec3 Plu::GameObject::GetObjectForwardVector() const
{
	return GetForwardVector(GetObjectRotation());
}

Vec3 Plu::GameObject::GetObjectRightVector() const
{
	return GetRightVector(GetObjectRotation());
}

Vec3 Plu::GameObject::GetObjectUpVector() const
{
	return GetUpVector(GetObjectRotation());
}

Plu::PluUUID& Plu::GameObject::GetObjectUUID()
{
	return mUuid;
}
