#pragma once
#include "EngineObject.h"
#include "PluEngine/Log.h"
#include "PluEngine/Objects/EngineObjectManager.h"

namespace Plu
{
    template<class T, typename ... Args>
EngineObjectHandle EngineObjectManager::CreateObject(Args &&...args)
    {
        static_assert(std::is_base_of_v<EngineObject, T>, "T must derive from EngineObject");

        UInt32 idx;
        if (mFreeList.IsEmpty()) {
            idx = mObjects.Size();
            mObjects.PushBack(nullptr);
            mObjects[idx] = new T(std::forward<Args>(args)...);
            mGenerations.PushBack(0);
        } else {
            idx = *mFreeList.end();
            mObjects[idx] = new T(std::forward<Args>(args)...);
            mFreeList.PopBack();
        }
        mObjects[idx]->mHandle = EngineObjectHandle{idx, mGenerations[idx], false};
        mObjects[idx]->mEventDispatcher = CreateOwning<EventDispatcher>();
        return EngineObjectHandle(idx, mGenerations[idx], false);
    }

    template<class T>
    Plu::TOwningPointer<T> EngineObjectManager::GetObjectAsOwner(const EngineObjectHandle handle)
    {
        if (!IsValid(handle)) return nullptr;
        return Plu::DynamicCast<T>(mObjects.At(handle.Index));
    }

    template<class T>
    Plu::TUsePointer<T> EngineObjectManager::GetObjectAsUser(const EngineObjectHandle handle)
    {
        if (!IsValid(handle)) return nullptr;
        return Plu::DynamicCast<T>(mObjects.At(handle.Index));
    }
}

