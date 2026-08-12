//
// Created by Plutex on 7/19/26.
//

#include "PluEngine/Gameplay/Components/InstancedStaticMeshComponent.h"

#include <cstring>

#include "PluEngine/Gameplay/GameObject.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

Plu::TUsePointer<Plu::StaticMesh> Plu::InstancedStaticMeshComponent::GetStaticMesh()
{
	return StaticMeshToDisplay;
}

void Plu::InstancedStaticMeshComponent::SetStaticMesh(TUsePointer<StaticMesh> staticMesh)
{
	StaticMeshToDisplay = staticMesh;
	MeshBoundingBoxComputed = false;
	if (staticMesh && staticMesh->IsLoaded) {
		MeshBoundingBox = Plu::CreateBoundingBoxForStaticMesh(staticMesh.GetRaw());
		MeshBoundingBoxComputed = true;
	}
}

Plu::TUsePointer<Plu::MaterialInfo> Plu::InstancedStaticMeshComponent::GetMaterial()
{
	return Material;
}

void Plu::InstancedStaticMeshComponent::SetMaterial(TUsePointer<MaterialInfo> material)
{
	Material = material;
}

Int32 Plu::InstancedStaticMeshComponent::AddInstance(Vec3 loc, Vec3 rot, Vec3 scale)
{
	MeshInstanceTransform& inst = Instances.EmplaceBack();
	inst.Location = loc;
	inst.Rotation = rot;
	inst.Scale = scale;
	mInstanceCacheDirty = true;
	return static_cast<Int32>(Instances.Size() - 1);
}

bool Plu::InstancedStaticMeshComponent::RemoveInstance(Int32 index)
{
	if (index < 0 || static_cast<UInt32>(index) >= Instances.Size()) return false;
	const UInt32 lastIndex = Instances.Size() - 1;
	if (static_cast<UInt32>(index) != lastIndex) {
		Instances[index] = Instances[lastIndex];
	}
	Instances.RemoveAt(lastIndex);
	mInstanceCacheDirty = true;
	return true;
}

bool Plu::InstancedStaticMeshComponent::UpdateInstance(Int32 index, Vec3 loc, Vec3 rot, Vec3 scale)
{
	if (index < 0 || static_cast<UInt32>(index) >= Instances.Size()) return false;
	MeshInstanceTransform& inst = Instances[index];
	inst.Location = loc;
	inst.Rotation = rot;
	inst.Scale = scale;
	mInstanceCacheDirty = true;
	return true;
}

void Plu::InstancedStaticMeshComponent::ClearInstances()
{
	Instances.Clear();
	mInstanceCacheDirty = true;
}

Int32 Plu::InstancedStaticMeshComponent::GetInstanceCount() const
{
	return static_cast<Int32>(Instances.Size());
}

void Plu::InstancedStaticMeshComponent::ReserveInstances(Int32 count)
{
	Instances.Reserve(static_cast<UInt32>(count));
}

void Plu::InstancedStaticMeshComponent::SetInstances(const DynamicArray<MeshInstanceTransform>& instances)
{
	Instances = instances;
	mInstanceCacheDirty = true;
}

const DynamicArray<Matrix4>* Plu::InstancedStaticMeshComponent::GetInstanceWorldMatrices()
{
	const Matrix4 componentWorld = GetWorldMatrix();

	// Rozmiar/zawartość porównane wprost: mInstanceCacheDirty łapie tylko AddInstance/RemoveInstance/
	// UpdateInstance/ClearInstances/SetInstances. Edycje z panelu detali (reflekcja) i deserializacja
	// sceny piszą po bajtach Instances z pominięciem tych metod, więc bez tego porównania cache
	// nigdy by się nie odświeżył po takiej edycji - w rezultacie liczba w panelu zmienia się, ale
	// zrenderowana instancja stoi w miejscu (wygląda jak "zmiana zignorowana").
	const bool instancesContentChanged = Instances.Size() != mCachedInstances.Size() ||
		(!Instances.IsEmpty() && std::memcmp(Instances.Data(), mCachedInstances.Data(), Instances.Size() * sizeof(MeshInstanceTransform)) != 0);

	if (mInstanceCacheDirty || instancesContentChanged || std::memcmp(&componentWorld, &mCachedComponentWorldMatrix, sizeof(Matrix4)) != 0) {
		mCachedWorldMatrices.Clear();
		mCachedWorldMatrices.Reserve(Instances.Size());
		mCachedNormalMatrices.Clear();
		mCachedNormalMatrices.Reserve(Instances.Size());
		for (const MeshInstanceTransform& inst : Instances) {
			const Matrix4 local = glm::translate(glm::mat4(1.0f), inst.Location) *
				glm::mat4_cast(glm::quat(glm::radians(inst.Rotation))) *
				glm::scale(glm::mat4(1.0f), inst.Scale);
			const Matrix4 world = componentWorld * local;
			mCachedWorldMatrices.PushBack(world);
			mCachedNormalMatrices.PushBack(glm::transpose(glm::inverse(world)));
		}
		mCachedComponentWorldMatrix = componentWorld;
		mCachedInstances = Instances;
		mInstanceCacheDirty = false;
	}
	return &mCachedWorldMatrices;
}

const DynamicArray<Matrix4>* Plu::InstancedStaticMeshComponent::GetInstanceNormalMatrices()
{
	GetInstanceWorldMatrices();
	return &mCachedNormalMatrices;
}
