#pragma once

namespace Plu
{
    // Struktura kontrolna z licznikami
    template<typename T>
    struct ControlBlock
    {
        T* ptr;
        int owningCount;
        int useCount;
        bool isPython;

        ControlBlock(T* ptr) : ptr(ptr), owningCount(1), useCount(0), isPython(false) {}
    };
}
