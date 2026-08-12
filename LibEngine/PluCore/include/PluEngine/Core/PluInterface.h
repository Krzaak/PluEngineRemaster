//
// Created by Plutex on 26.01.2026.
//

#ifndef PLUENGINE_PLUINTERFACE_H
#define PLUENGINE_PLUINTERFACE_H

#include "PluEngine/Core.h"
#include "PluEngine/Core/Reflection/ReflectionBase.h"

namespace Plu
{
    PLU_INTERFACE()
    class PLUCORE_API PluInterface
    {
    public:
        virtual ~PluInterface() = default;
        static TypeInfo* GetStaticClass();
        virtual TypeInfo* GetClass() = 0;
    };
}

#endif //PLUENGINE_PLUINTERFACE_H