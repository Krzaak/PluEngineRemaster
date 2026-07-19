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

void Plu::WorldComponent::MarkWorldMatrixForRegeneration()
{
	mRegenerateWorldMatrix = true;
	mRegenerateNormalMatrix = true;
	for (auto child : mWorldComponents) {
		child->MarkWorldMatrixForRegeneration();
	}
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

Matrix4 Plu::WorldComponent::GetWorldMatrix()
{
	if (mRegenerateWorldMatrix) {
		mRegenerateWorldMatrix = false;
		Matrix4 localMatrix = glm::translate(glm::mat4(1.0f), GetRelativeLocation()) *
			  glm::mat4_cast(glm::quat(glm::radians(GetRelativeRotation()))) *
			  glm::scale(glm::mat4(1.0f), GetRelativeScale());
		if (mParentComponent) {
			mWorldMatrix = localMatrix * mParentComponent->GetWorldMatrix();
		} else {
			mWorldMatrix = GetParentGameObject()->GetObjectWorldMatrix() * localMatrix;
		}
	}
	return mWorldMatrix;
}

Matrix4 Plu::WorldComponent::GetNormalMatrix()
{
	if (mRegenerateNormalMatrix) {
		mRegenerateNormalMatrix = false;
		mNormalMatrix = glm::transpose(glm::inverse(GetWorldMatrix()));
	}
	return mNormalMatrix;
}

Vec3 Plu::WorldComponent::GetRelativeLocation()
{
	return mRelativeLocation;
}

Vec3 Plu::WorldComponent::GetRelativeRotation()
{
	return mRelativeRotation;
}

Vec3 Plu::WorldComponent::GetRelativeScale()
{
	return mRelativeScale;
}

void Plu::WorldComponent::SetRelativeLocation(Vec3 newLoc)
{
	mRelativeLocation = newLoc;
	MarkWorldMatrixForRegeneration();
}

void Plu::WorldComponent::SetRelativeRotation(Vec3 newRot)
{
	mRelativeRotation = newRot;
	MarkWorldMatrixForRegeneration();
}

void Plu::WorldComponent::SetRelativeScale(Vec3 newScale)
{
	mRelativeScale = newScale;
	MarkWorldMatrixForRegeneration();
}

Vec3 Plu::WorldComponent::GetWorldLocation()
{
	return Vec3(GetWorldMatrix()[3]);
}

Vec3 Plu::WorldComponent::GetWorldScale()
{
	Matrix4 m = GetWorldMatrix();
	return Vec3(
		glm::length(Vec3(m[0])),
		glm::length(Vec3(m[1])),
		glm::length(Vec3(m[2]))
	);
}

Vec3 Plu::WorldComponent::GetWorldRotation()
{
	Matrix4 m = GetWorldMatrix();
	Vec3 scale = GetWorldScale();
	glm::mat3 rotMat = glm::mat3(
		Vec3(m[0]) / scale.x,
		Vec3(m[1]) / scale.y,
		Vec3(m[2]) / scale.z
	);
	return glm::degrees(glm::eulerAngles(glm::quat_cast(rotMat)));
}

void Plu::WorldComponent::SetWorldLocation(Vec3 newLoc)
{
	Matrix4 parentWorld = (mParentComponent != nullptr)
		? mParentComponent->GetWorldMatrix()
		: GetParentGameObject()->GetObjectWorldMatrix();

	Vec3 localPos = Vec3(glm::inverse(parentWorld) * Vec4(newLoc, 1.0f));
	SetRelativeLocation(localPos);
}

void Plu::WorldComponent::SetWorldRotation(Vec3 newRot)
{
	Quaternion worldRot = Quaternion(glm::radians(newRot));
	Quaternion parentWorldRot;

	if (mParentComponent != nullptr)
		parentWorldRot = Quaternion(glm::radians(mParentComponent->GetWorldRotation()));
	else
		parentWorldRot = Quaternion(glm::radians(GetParentGameObject()->GetObjectRotation()));

	Quaternion localRot = glm::inverse(parentWorldRot) * worldRot;
	SetRelativeRotation(glm::degrees(glm::eulerAngles(localRot)));
}

void Plu::WorldComponent::SetWorldScale(Vec3 newScale)
{
	Vec3 parentScale = (mParentComponent != nullptr)
		? mParentComponent->GetWorldScale()
		: GetParentGameObject()->GetObjectScale();

	SetRelativeScale(newScale / parentScale);
}

JSON Plu::WorldComponent::Serialize()
{
	JSON j = TypeSerializer<TypeInfo*>::Serialize(this->GetClass(), this);
	j["children"] = JSON::array();
	for (const auto& child : mWorldComponents) {
		j["children"].push_back(child->Serialize());
	}
	Vec3 relativeLocation = GetRelativeLocation();
	Vec3 relativeRotation = GetRelativeRotation();
	Vec3 relativeScale = GetRelativeScale();
	j["relativeLocation"] = TypeSerializer<Vec3>::Serialize(&relativeLocation);
	j["relativeRotation"] = TypeSerializer<Vec3>::Serialize(&relativeRotation);
	j["relativeScale"] = TypeSerializer<Vec3>::Serialize(&relativeScale);
	return j;
}
