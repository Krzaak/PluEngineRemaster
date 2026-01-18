//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Objects/EngineObjectManager.h"

#include "PluEngine/Objects/EngineObject.h"

using namespace Plu;

EngineObjectManager::EngineObjectManager()
{
	PLU_CORE_WARN("ObjectManager created");
	mObjects.Reserve(1000);
}

EngineObjectManager::~EngineObjectManager()
{
	PLU_CORE_WARN("ObjectManager destroyed");
	MaxInt64 numObjects = mObjects.Size();
	for (MaxInt64 i = 0; i < numObjects; ++i) {
		mObjects[i] = nullptr;
	}
	mObjects.Clear();
}

DynamicArray<String> EngineObjectManager::GetObjectNames(MaxUInt32 numElements)
{
	DynamicArray<String> names;
	MaxUInt32 numObjects = mObjects.Size();
	GameHashMap<String, MaxInt16> visited;
	MaxUInt64 nullptrIds = 0;
	for (MaxUInt32 i = 0; i < numElements && i < numObjects; ++i) {
		if (!mObjects[i]) {
			names.PushBack("nullptr" + String::FromInt(nullptrIds));
			nullptrIds++;
			continue;
		}
		MaxInt16 id = 0;
		if (visited.Contains(mObjects[i]->GetClass()->TypeName.CStr())) {
			++visited[mObjects[i]->GetClass()->TypeName.CStr()];
			id = visited[mObjects[i]->GetClass()->TypeName.CStr()];
		} else {
			visited.Insert(mObjects[i]->GetClass()->TypeName.CStr(), 0);
		}
		names.PushBack(mObjects[i]->GetClass()->TypeName + String::FromInt(id));
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
	mObjects[idx]->mHandle = hdl;
	return GetObjectAsOwner<EngineObject>(hdl);
}

void EngineObjectManager::DestroyObject(const EngineObjectHandle &handle)
{
	if (!IsValid(handle)) return;
	const MaxInt32 id = handle.Index;
	mObjects[id] = nullptr;
	mFreeList.PushBack(id);
	++mGenerations[id];
}

bool EngineObjectManager::IsValid(const EngineObjectHandle &handle)
{
	return handle.Index < mObjects.Size() && mObjects[handle.Index] != nullptr && handle.Generation == mGenerations[handle.Index] && !handle.failed;
}
