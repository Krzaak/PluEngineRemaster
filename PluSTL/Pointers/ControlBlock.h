#pragma once
#include <atomic>

namespace Plu
{
    // Refcount control block shared by TOwningPointer (strong) and TUsePointer (weak).
    //
    // Thread-safety follows the canonical shared_ptr protocol so that owners/observers on
    // different threads can release concurrently without racing on the object or the block:
    //
    //   strongCount = number of owners. When it reaches 0 the managed object is destroyed.
    //   weakCount   = number of use-pointers + 1. The extra +1 is a single collective weak
    //                 reference held by all owners together; it is dropped when the last owner
    //                 goes away. When weakCount reaches 0 the control block itself is freed.
    //
    // Exactly one counter (weakCount) decides the block's lifetime, which removes the
    // check-then-act double-free of the old two-counter design. Increments are relaxed (the
    // caller already holds a live reference); the decrement that reaches 0 is acq_rel so the
    // destroying thread observes every prior modification through the released object.
    struct ControlBlockBase
    {
        void* ptr;
        std::atomic<int> strongCount{1};
        std::atomic<int> weakCount{1};
        bool isPython = false;

        explicit ControlBlockBase(void* ptr) : ptr(ptr) {}
        virtual ~ControlBlockBase() = default;
    };

    template<typename T>
    struct ControlBlock : ControlBlockBase
    {
        explicit ControlBlock(T* p) : ControlBlockBase(p) {}
        T* Get() const { return static_cast<T*>(ptr); }
    };
}
