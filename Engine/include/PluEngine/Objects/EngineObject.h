//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_ENGINEOBJECT_H
#define PLUENGINE_ENGINEOBJECT_H
#include "PluEngine/Core.h"
#include "EngineObject.generated.h"
#include "EngineObjectHandle.h"
#include "PluEngine/Events/EventDispatcher.h"

namespace Plu
{
    PLU_CLASS(Abstract, PyExport, PyDerive)
    class PLU_API EngineObject
    {
    private:
        friend class EngineObjectManager;
        EngineObjectHandle mHandle = {};
        UInt32 mShortTermID = 0;
        TOwningPointer<EventDispatcher> mEventDispatcher;
        TypeInfo* mPythonType = nullptr;
        template<typename B>
        requires EngineObjectConc<B>
        friend TOwningPointer<B> OwnerFromPython(pybind11::type type);
    protected:
        TypeInfo* GetPythonType() const
        {return mPythonType;}
        void DispatchEvent(const String& name, void* payload) const {mEventDispatcher->Dispatch(name, payload);}
    public:
        TUsePointer<EventDispatcher> GetObjectEventDispatcher() {return mEventDispatcher;}
        EngineObjectHandle* GetEngineObjectHandle() {return &mHandle;}
        static TypeInfo* GetStaticClass();
        virtual TypeInfo* GetClass() = 0;
        virtual ~EngineObject() = default;
        String GetDisplayName() //ClassName + Short-Term ID
        {
            TypeInfo* type = GetClass();
            return type->TypeName + String::FromInt(mShortTermID);
        }
    };
}

#endif //PLUENGINE_ENGINEOBJECT_H