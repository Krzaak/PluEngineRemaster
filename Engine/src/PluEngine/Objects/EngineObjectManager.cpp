//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Objects/EngineObjectManager.h"

#include "PluEngine/Objects/EngineObject.h"
#include "PluEngine/Python/PythonPointers.h"

using namespace Plu;

EngineObjectManager::EngineObjectManager()
{
	PLU_CORE_WARN("ObjectManager created");
	mObjects.Reserve(1000);
}

EngineObjectManager::~EngineObjectManager()
{
	PLU_CORE_WARN("ObjectManager destroyed");
	Int64 numObjects = mObjects.Size();
	for (Int64 i = 0; i < numObjects; ++i) {
		mObjects[i] = nullptr;
	}
	mObjects.Clear();
}

DynamicArray<String> EngineObjectManager::GetObjectNames(UInt32 numElements)
{
	DynamicArray<String> names;
	UInt32 numObjects = mObjects.Size();
	UInt64 nullptrIds = 0;
	for (UInt32 i = 0; i < numElements && i < numObjects; ++i) {
		if (!mObjects[i]) {
			names.PushBack("nullptr" + String::FromInt(nullptrIds));
			nullptrIds++;
			continue;
		}
		names.PushBack(mObjects[i]->GetDisplayName());
	}
	return names;
}

UInt32 EngineObjectManager::GetNumberOfObjects()
{
	return mObjects.Size();
}

TUsePointer<EngineObject> EngineObjectManager::GetObjectOnIndex(UInt32 idx)
{
	if (idx < mObjects.Size()) {
		try {
			return mObjects[idx];
		} catch (...) {
			PLU_CORE_ERROR("Couldn't get object at index {}", idx);
		}
	}
	return nullptr;
}

DynamicArray<TUsePointer<EngineObject>> EngineObjectManager::GetAllObjectsOfClass(TypeInfo* parent)
{
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	DynamicArray<TUsePointer<EngineObject>> childObjs;
	//Let's fucking GO!
	for (const TOwningPointer<EngineObject>& obj : mObjects) {
		if (!obj) continue;
		TypeInfo* classOfObj = obj->GetClass();
		if (classOfObj->IsDerivedOfOrSame(parent)) {
			childObjs.PushBack(obj);
		}
	}
	float time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start).count();
	PLU_CORE_INFO("Getting all objects of type: {} took {}", parent->TypeName.CStr(), time);
	return childObjs;
}

TUsePointer<EngineObject> EngineObjectManager::CreateObject(const TypeInfo *Class)
{
	UInt32 idx;
	if (mFreeList.IsEmpty()) {
		idx = mObjects.Size();
		mObjects.PushBack(nullptr);
		if (Class->IsPythonType) {
			mObjects[idx] = OwnerFromPython<EngineObject>(Class->PythonType);
		} else {
			mObjects[idx] = TOwningPointer(static_cast<EngineObject*>(Class->Construct()));
		}
		mGenerations.PushBack(0);
	} else {
		idx = mFreeList.Back();
		if (Class->IsPythonType) {
			mObjects[idx] = OwnerFromPython<EngineObject>(Class->PythonType);
		} else {
			mObjects[idx] = TOwningPointer(static_cast<EngineObject*>(Class->Construct()));
		}
		mFreeList.PopBack();
	}
	const EngineObjectHandle hdl = EngineObjectHandle(idx, mGenerations[idx], false);
	mObjects[idx]->mHandle = hdl;
	mObjects[idx]->mEventDispatcher = CreateOwning<EventDispatcher>();
	EngineObject* obj = mObjects[idx].Get();
	TypeInfo* type = obj->GetClass();
	const String typeName = type->TypeName;
	if (mShortTermIDs.Contains(typeName))
	{
		mShortTermIDs[typeName]++;
	} else
	{
		mShortTermIDs[typeName] = 0;
	}
	mObjects[idx]->mShortTermID = mShortTermIDs[typeName];
	return GetObjectAsOwner<EngineObject>(hdl);
}

void EngineObjectManager::DestroyObject(const EngineObjectHandle &handle)
{
	if (!IsValid(handle)) return;
	const Int32 id = handle.Index;
	mObjects[id] = nullptr;
	mFreeList.PushBack(id);
	++mGenerations[id];
}

bool EngineObjectManager::IsValid(const EngineObjectHandle &handle)
{
	return handle.Index < mObjects.Size() && mObjects[handle.Index] != nullptr && handle.Generation == mGenerations[handle.Index] && !handle.failed;
}
