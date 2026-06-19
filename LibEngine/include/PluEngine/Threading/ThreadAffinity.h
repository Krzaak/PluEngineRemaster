//
// Created by Plutex.
//
// Thread-affinity helpers for the multithreading rollout.
// The engine confines mutation of core subsystems (EngineObjectManager,
// AssetManager registry, SceneWorld, Input, Python) to the main thread; the
// render thread reads snapshots only. These helpers identify the main thread so
// confinement can be asserted in debug builds (see PLU_CORE_ASSERT).
//

#ifndef PLUENGINE_THREADAFFINITY_H
#define PLUENGINE_THREADAFFINITY_H

#include "PluEngine/Core.h"

#include <thread>

namespace Plu
{
    // Records the calling thread as the engine's main thread. Call once, on the
    // main thread, during engine initialization.
    PLU_API void RegisterMainThread();

    // The id captured by RegisterMainThread (default-constructed id if never called).
    PLU_API std::thread::id GetMainThreadId();

    // True if the calling thread is the registered main thread. Returns true when
    // no main thread has been registered yet, so pre-init code and standalone
    // tooling never trip confinement asserts.
    PLU_API bool IsOnMainThread();
}

#endif //PLUENGINE_THREADAFFINITY_H
