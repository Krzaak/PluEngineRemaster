//
// Created by Plutex on 12/30/25.
//

#ifndef PLUENGINE_ENGINEOBJECTHANDLE_H
#define PLUENGINE_ENGINEOBJECTHANDLE_H
#include "PluEngine/Core.h"

namespace Plu
{
    struct EngineObjectHandle
    {
        MaxUInt32 Index;
        MaxUInt32 Generation;
        bool failed = true;
    };
}

#endif //PLUENGINE_ENGINEOBJECTHANDLE_H