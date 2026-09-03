//
// Created by Plutex on 12/30/25.
//

#include "PluEngine/Core/Objects/EngineObjectManager.h"

#include "PluEngine/Core/Objects/EngineObject.h"

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
	// Use-only reader (any thread): observe each slot through a TUsePointer so the
	// deref never goes through the owning operator-> (which asserts owner-thread).
	std::shared_lock lock(mMutex);
	DynamicArray<String> names;
	UInt32 numObjects = mObjects.Size();
	UInt64 nullptrIds = 0;
	for (UInt32 i = 0; i < numElements && i < numObjects; ++i) {
		if (!mObjects[i]) {
			names.PushBack("nullptr" + String::FromInt(nullptrIds));
			nullptrIds++;
			continue;
		}
		TUsePointer<EngineObject> use(mObjects[i]);
		names.PushBack(use->GetDisplayName());
	}
	return names;
}

UInt32 EngineObjectManager::GetNumberOfObjects()
{
	std::shared_lock lock(mMutex);
	return mObjects.Size();
}

TUsePointer<EngineObject> EngineObjectManager::GetObjectOnIndex(UInt32 idx)
{
	// Returning an owning slot into a TUsePointer return type goes through the
	// owning->use converting ctor (weakCount only), so this is already use-only.
	std::shared_lock lock(mMutex);
	if (idx < mObjects.Size()) {
		try {
			return mObjects[idx];
		} catch (...) {
			PLU_CORE_ERROR("Couldn't get object at index {}", idx);
		}
	}
	return nullptr;
}

DynamicArray<EngineObjectHandle> EngineObjectManager::GetAllObjectsOfClass(TypeInfo* parent)
{
	// Slow editor-only sweep; main-thread-only (derefs owning pointers). shared_lock
	// guards the iteration against a concurrent writer reallocating the slot-map.
	CheckOwnerThread();
	std::shared_lock lock(mMutex);
	std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
	DynamicArray<EngineObjectHandle> childObjs;
	//Let's fucking GO!
	for (const TOwningPointer<EngineObject>& obj : mObjects) {
		if (!obj) continue;
		// Observe through a use-pointer: the slot may be owned by another thread (e.g. a
		// render-owned Texture), so the owning operator-> would trip the affinity assert.
		TUsePointer<EngineObject> use(obj);
		TypeInfo* classOfObj = use->GetClass();
		if (classOfObj->IsDerivedOfOrSame(parent)) {
			childObjs.PushBack(*use->GetEngineObjectHandle());
		}
	}
	float time = std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start).count();
	PLU_CORE_INFO("Getting all objects of type: {} took {}", parent->TypeName.CStr(), time);
	return childObjs;
}

std::function<TOwningPointer<EngineObject>(pybind11::type)> EngineObjectManager::PythonObjectFactory;

TUsePointer<EngineObject> EngineObjectManager::CreateObject(const TypeInfo *Class)
{
	// Construct OUTSIDE the lock — Construct()/Python instantiation may itself create
	// engine objects, and the slot-map mutex is non-recursive (see CreateObject<T>).
	TOwningPointer<EngineObject> created;
	if (Class->IsPythonType) {
		PLU_CORE_ASSERT(EngineObjectManager::PythonObjectFactory,
			"Python type requested but the scripting layer never installed its object factory")
		created = EngineObjectManager::PythonObjectFactory(Class->PythonType);
	} else {
		created = TOwningPointer(static_cast<EngineObject*>(Class->Construct()));
	}

	std::unique_lock lock(mMutex);
	UInt32 idx;
	if (mFreeList.IsEmpty()) {
		idx = mObjects.Size();
		mObjects.PushBack(nullptr);
		mObjects[idx] = std::move(created);
		mGenerations.PushBack(0);
	} else {
		idx = mFreeList.Back();
		mObjects[idx] = std::move(created);
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
	// Already holding unique_lock — use the unlocked getter to avoid re-entering mMutex.
	return GetObjectAsOwnerUnlocked<EngineObject>(hdl);
}

void EngineObjectManager::DestroyObject(const EngineObjectHandle &handle)
{
	// Detach the slot under the lock, then drop the owning ref AFTER releasing it.
	// ~EngineObject may recursively destroy children (or otherwise re-enter the manager),
	// and std::shared_mutex is non-recursive — running the destructor while holding the
	// lock would deadlock/crash. Move the ref out, update bookkeeping, unlock, then let
	// `condemned` fall out of scope to run the (possibly recursive) destruction unlocked.
	TOwningPointer<EngineObject> condemned;
	{
		std::unique_lock lock(mMutex);
		if (!IsValidUnlocked(handle)) return;
		const Int32 id = handle.Index;
		condemned = std::move(mObjects[id]);
		mFreeList.PushBack(id);
		++mGenerations[id];
	}
	// lock released — destruction (and any re-entrant DestroyObject) is now safe.
}

bool EngineObjectManager::IsValidUnlocked(const EngineObjectHandle &handle) const
{
	return handle.Index < mObjects.Size() && mObjects[handle.Index] != nullptr && handle.Generation == mGenerations[handle.Index] && !handle.failed;
}

bool EngineObjectManager::IsValid(const EngineObjectHandle &handle)
{
	// Use-only reader: callable from any thread.
	std::shared_lock lock(mMutex);
	return IsValidUnlocked(handle);
}
