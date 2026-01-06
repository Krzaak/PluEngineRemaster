//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_ENGINEOBJECT_H
#define PLUENGINE_ENGINEOBJECT_H
#include "PluEngine/Core.h"
#include "String/String.h"
#include "EngineObject.generated.h"

namespace Plu
{
    PLU_CLASS(Abstract)
    class PLU_API EngineObject
    {
    public:
        static TypeInfo* GetStaticClass();
        virtual TypeInfo* GetClass() = 0;
        virtual ~EngineObject() = default;
    };
}

#endif //PLUENGINE_ENGINEOBJECT_H