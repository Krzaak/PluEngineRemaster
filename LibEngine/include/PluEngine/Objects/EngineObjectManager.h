//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_ENGINEOBJECTMANAGER_H
#define PLUENGINE_ENGINEOBJECTMANAGER_H

#include <PluSTL_FWD.h>

#include "EngineObjectHandle.h"
#include "PluEngine/Core.h"
#include "PluEngine/Threading/ThreadAffinity.h"

namespace Plu
{
    struct TypeInfo;
    class EngineObject;

    class PLU_API EngineObjectManager
    {
        DynamicArray<TOwningPointer<EngineObject>> mObjects;
        DynamicArray<UInt32> mGenerations;
        DynamicArray<UInt32> mFreeList;
        GameHashMap<String, UInt32> mShortTermIDs;

        // Thread confinement (MT etap 02): the manager is main-thread-only — the
        // render thread reads snapshots, never the live slot-map. Asserts in debug,
        // compiles away in release. See PluEngine/Threading/ThreadAffinity.h.
        void CheckOwnerThread() const
        {
            PLU_CORE_ASSERT(IsOnMainThread(), "EngineObjectManager accessed off the main thread");
        }
    public:
        EngineObjectManager();
        ~EngineObjectManager();

        DynamicArray<String> GetObjectNames(UInt32 numElements);
        UInt32 GetNumberOfObjects();
        TUsePointer<EngineObject> GetObjectOnIndex(UInt32 idx);

        //Refactor definition
        //SLOW!!!!!! DO NOT USE EVERY FRAME!!!! ITERATES THROUGH ALL OBJECTS AND CHECKS PARENTS!!! THOUSANDS OF OPERATIONS!!!
        DynamicArray<EngineObjectHandle> GetAllObjectsOfClass(TypeInfo* parent);

        template<class T, typename... Args>
        EngineObjectHandle CreateObject(Args&&... args);

        TUsePointer<EngineObject> CreateObject(const TypeInfo *Class);

        template<class T>
        Plu::TOwningPointer<T> GetObjectAsOwner(EngineObjectHandle handle);

        template<class T>
        Plu::TUsePointer<T> GetObjectAsUser(EngineObjectHandle handle);

        void DestroyObject(const EngineObjectHandle &handle);
        bool IsValid(const EngineObjectHandle &handle);
    };
}

#include "EngineObjectManager.inl"

#endif //PLUENGINE_ENGINEOBJECTMANAGER_H