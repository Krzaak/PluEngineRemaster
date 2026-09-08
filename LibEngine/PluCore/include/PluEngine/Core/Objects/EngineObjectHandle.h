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
        UInt32 Index = 0;
        UInt32 Generation = 0;
        bool failed = true;

        bool operator==(const EngineObjectHandle& other) const
        {
            return Index == other.Index && Generation == other.Generation && failed == other.failed;
        }

        [[nodiscard]] String ToString() const
        {
            return String::Format("Handle: gen: {0}, idx: {1}", Generation, Index);
        }
    };

    template<>
    struct DefaultHash<EngineObjectHandle> {
        std::size_t operator()(const EngineObjectHandle& hdl) const noexcept {
            std::size_t idxHash = DefaultHash<UInt32>{}(hdl.Index);
            std::size_t genHash = DefaultHash<UInt32>{}(hdl.Generation);
            std::size_t failHash = DefaultHash<UInt32>{}(hdl.failed ? 0xFFFFFFFF : 0);

            std::size_t hash = idxHash ^ genHash;
            hash *= failHash;

            return hash;
        }
    };
}

#endif //PLUENGINE_ENGINEOBJECTHANDLE_H