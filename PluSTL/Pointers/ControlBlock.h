#pragma once

namespace Plu
{
    struct ControlBlockBase
    {
        void* ptr;
        int owningCount = 1;
        int useCount    = 0;
        bool isPython   = false;

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