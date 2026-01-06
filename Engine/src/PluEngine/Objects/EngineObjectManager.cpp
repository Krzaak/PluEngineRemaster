//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Objects/EngineObjectManager.h"

#include "PluEngine/Objects/EngineObject.h"

using namespace Plu;

EngineObjectManager::EngineObjectManager()
{
	PLU_CORE_WARN("ObjectManager created");
}

EngineObjectManager::~EngineObjectManager()
{
	PLU_CORE_WARN("ObjectManager destroyed");
}

DynamicArray<String> EngineObjectManager::GetObjectNames(MaxUInt32 numElements)
{
	DynamicArray<String> names;
	MaxUInt32 numObjects = mObjects.Size();
	for (MaxUInt32 i = 0; i < numElements && i < numObjects; ++i) {
		//names.PushBack(("Object_" + std::to_string(i)).data());
		names.PushBack(mObjects[i]->GetClass()->TypeName);
	}
	return names;
}

MaxUInt32 EngineObjectManager::GetNumberOfObjects()
{
	return mObjects.Size();
}

TOwningPointer<EngineObject> EngineObjectManager::CreateObject(const TypeInfo *Class)
{
	MaxUInt32 idx;
	if (mFreeList.IsEmpty()) {
		idx = mObjects.Size();
		mObjects.PushBack(nullptr);
		mObjects[idx] = TOwningPointer(static_cast<EngineObject*>(Class->Construct()));
		mGenerations.PushBack(0);
	} else {
		idx = *mFreeList.end();
		mObjects[idx] = TOwningPointer(static_cast<EngineObject*>(Class->Construct()));
		mFreeList.PopBack();
	}
	const EngineObjectHandle hdl = EngineObjectHandle(idx, mGenerations[idx], false);
	return GetObjectAsOwner<EngineObject>(hdl);
}

void EngineObjectManager::DestroyObject(const EngineObjectHandle &handle)
{
	if (!IsValid(handle)) return;
	mObjects[handle.Index] = nullptr;
	mFreeList.PushBack(handle.Index);
	++mGenerations[handle.Index];
}

bool EngineObjectManager::IsValid(const EngineObjectHandle &handle)
{
	return handle.Index < mObjects.Size() && mObjects[handle.Index] != nullptr && handle.Generation == mGenerations[handle.Index];
}
