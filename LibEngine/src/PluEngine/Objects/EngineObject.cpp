//
// Created by Plutex.
//

#include "PluEngine/Objects/EngineObject.h"

#include "PluEngine/Log.h"
#include "PluEngine/Threading/ThreadAffinity.h"

namespace Plu
{
    // Lifetime confinement (MT etap 02): engine objects are created and destroyed
    // on the main thread only. If the render thread ever held the last owning
    // pointer, the object would be deleted there — this catches it in debug.
    EngineObject::~EngineObject()
    {
        PLU_CORE_ASSERT(IsOnMainThread(),
            "EngineObject destroyed off the main thread (lifetime confinement violated)");
    }
}
