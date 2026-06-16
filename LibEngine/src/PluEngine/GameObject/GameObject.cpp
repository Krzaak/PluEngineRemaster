//
// Created by Plutex on 1/18/26.
//

#include "PluEngine/GameObject/GameObject.h"

#include "PluEngine/PluUtils.h"
#include "PluEngine/GameObject/GameObjectComponent.h"
#include "PluEngine/GameObject/WorldComponent.h"
#include "PluEngine/Input/InputHandler.h"
#include "PluEngine/Managers/ScenesManager.h"
#include "PluEngine/Objects/EngineObjectManager.h"
#include "PluEngine/Physics/PhysicsBody.h"
#include "PluEngine/Physics/PhysicsCompoundShape.h"
#include "PluEngine/Scenes/SceneWorld.h"

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
	mObjectManager->DestroyObject(mPhysicsBodyHandle);
	if (mCompoundShape) {
		mObjectManager->DestroyObject(*mCompoundShape->GetEngineObjectHandle());
	}
	mCompoundShape = nullptr;
}

void Plu::GameObject::TickObject(float deltaTime)
{
	if (GetInputHandler()) {
		GetInputHandler()->TickHandler();
	}
	try
	{
		OnUpdate(deltaTime);
	} catch (pybind11::error_already_set& e)
	{
		PLU_CORE_ERROR("Error In Python {}", e.what());
	}
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
	TUsePointer<GameObjectComponent> newComponent = mObjectManager->CreateObject(componentClass);
	newComponent->mComponentName = componentName;
	RegisterComponent(*newComponent->GetEngineObjectHandle());
	return newComponent;
}

void Plu::GameObject::RegisterComponent(EngineObjectHandle component)
{
	TOwningPointer<GameObjectComponent> newComponent = mObjectManager->GetObjectAsOwner<GameObjectComponent>(component);
	if (newComponent->GetClass()->IsDerivedOfOrSame(WorldComponent::GetStaticClass())) {
		mWorldComponents.PushBack(newComponent);
		mRedoWorldComponentList = true;
	} else {
		mComponents.PushBack(newComponent);
	}
	newComponent->SetParentGameObject(mObjectManager->GetObjectAsUser<GameObject>(*GetEngineObjectHandle()));
	mWorld->NewGameObjectComponent(newComponent);
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

Plu::TUsePointer<Plu::GameObjectComponent> Plu::GameObject::GetComponentByClass(
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

Matrix4 Plu::GameObject::GetObjectWorldMatrix()
{
	if (mRegenerateWorldMatrix) {
		mRegenerateWorldMatrix = false;
		Matrix4 model = glm::translate(glm::mat4(1.0f), GetObjectLocation()) *
				  glm::mat4_cast(glm::quat(glm::radians(GetObjectRotation()))) *
				  glm::scale(glm::mat4(1.0f), GetObjectScale());
		mWorldMatrix = model;
	}
	return mWorldMatrix;
}

void Plu::GameObject::SetObjectLocation(const Vec3 &location)
{
	mLocation = location;
	GetObjectEventDispatcher()->Dispatch("LocationChange");
	mRegenerateWorldMatrix = true;
	for (auto child : mWorldComponents) {
		child->MarkWorldMatrixForRegeneration();
	}
	if (mObjectManager->IsValid(mPhysicsBodyHandle)) {
		mObjectManager->GetObjectAsUser<PhysicsBody>(mPhysicsBodyHandle)->SetPosition(ToJPH(GetObjectLocation()));
	}
}

void Plu::GameObject::SetObjectRotation(const Vec3 &rotation)
{
	mRotation = rotation;
	NormalizeVec3Rotation(&mRotation);
	GetObjectEventDispatcher()->Dispatch("RotationChange");
	mRegenerateWorldMatrix = true;
	for (auto child : mWorldComponents) {
		child->MarkWorldMatrixForRegeneration();
	}
	if (mObjectManager->IsValid(mPhysicsBodyHandle)) {
		mObjectManager->GetObjectAsUser<PhysicsBody>(mPhysicsBodyHandle)->SetRotation(JPH::Quat::sEulerAngles(
			JPH::Vec3(
				JPH::DegreesToRadians(mRotation.x),
				JPH::DegreesToRadians(mRotation.y),
				JPH::DegreesToRadians(mRotation.z)
			)
		));
	}
}

void Plu::GameObject::SetObjectScale(const Vec3 &scale)
{
	mScale = scale;
	GetObjectEventDispatcher()->Dispatch("ScaleChange");
	mRegenerateWorldMatrix = true;
	for (auto child : mWorldComponents) {
		child->MarkWorldMatrixForRegeneration();
	}
	// Jolt collision shapes are baked at a fixed scale; rebuild the body so colliders match.
	if (mWorld) {
		mWorld->OnGameObjectScaleChanged(this);
	}
}

void Plu::GameObject::SyncFromPhysicsBody(const Vec3& worldLocation, const Vec3& worldRotationDeg)
{
	mLocation = worldLocation;
	mRotation = worldRotationDeg;
	mRegenerateWorldMatrix = true;
	for (auto child : mWorldComponents) {
		child->MarkWorldMatrixForRegeneration();
	}
}

Plu::TUsePointer<Plu::PhysicsBody> Plu::GameObject::GetPhysicsBody()
{
	if (!mObjectManager->IsValid(mPhysicsBodyHandle))
		return nullptr;
	return mObjectManager->GetObjectAsUser<PhysicsBody>(mPhysicsBodyHandle);
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

