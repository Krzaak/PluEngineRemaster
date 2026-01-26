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
        UInt32 Index;
        UInt32 Generation;
        bool failed = true;

        bool operator==(const EngineObjectHandle& other) const
        {
            return Index == other.Index && Generation == other.Generation && failed == other.failed;
        }
    };
}

#endif //PLUENGINE_ENGINEOBJECTHANDLE_H