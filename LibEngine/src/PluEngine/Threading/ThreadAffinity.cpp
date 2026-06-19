//
// Created by Plutex.
//

#include "PluEngine/Threading/ThreadAffinity.h"

namespace Plu
{
    namespace
    {
        std::thread::id gMainThreadId{};
    }

    void RegisterMainThread()
    {
        gMainThreadId = std::this_thread::get_id();
    }

    std::thread::id GetMainThreadId()
    {
        return gMainThreadId;
    }

    bool IsOnMainThread()
    {
        // Not registered yet -> don't trip confinement asserts (pre-init / tooling).
        return gMainThreadId == std::thread::id{} || std::this_thread::get_id() == gMainThreadId;
    }
}
