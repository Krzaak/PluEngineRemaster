#pragma once
#include <new>

// ============================================================================
// DEFAULT ALLOCATOR INTERFACE
// ============================================================================

template<typename T>
class DefaultAllocator {
public:
    using ValueType = T;

    DefaultAllocator() noexcept = default;

    [[nodiscard]] T* Allocate(std::size_t count) noexcept {
        return static_cast<T*>(::operator new(count * sizeof(T), std::nothrow));
    }

    void Deallocate(T* ptr, std::size_t count) noexcept {
        ::operator delete(ptr, count * sizeof(T));
    }

    // Required for allocator interface
    template<typename... Args>
    void Construct(T* ptr, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        new(ptr) T(std::forward<Args>(args)...);
    }

    void Destroy(T* ptr) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            ptr->~T();
        }
    }
};

// ============================================================================
// LINEAR ALLOCATOR (Example custom allocator for engines)
// ============================================================================

template<typename T>
class LinearAllocator {
public:
    using ValueType = T;

    explicit LinearAllocator(std::size_t bufferSize) noexcept
        : m_Buffer(static_cast<std::byte*>(::operator new(bufferSize, std::nothrow)))
        , m_BufferSize(bufferSize)
        , m_Offset(0) {}

    ~LinearAllocator() noexcept {
        if (m_Buffer) {
            ::operator delete(m_Buffer);
        }
    }

    // Move-only
    LinearAllocator(const LinearAllocator&) = delete;
    LinearAllocator& operator=(const LinearAllocator&) = delete;
    LinearAllocator(LinearAllocator&& other) noexcept
        : m_Buffer(other.m_Buffer)
        , m_BufferSize(other.m_BufferSize)
        , m_Offset(other.m_Offset) {
        other.m_Buffer = nullptr;
    }

    [[nodiscard]] T* Allocate(std::size_t count) noexcept {
        const std::size_t size = count * sizeof(T);
        const std::size_t alignment = alignof(T);

        // Align offset
        std::size_t aligned = (m_Offset + alignment - 1) & ~(alignment - 1);

        if (aligned + size > m_BufferSize) {
            return nullptr; // Out of memory
        }

        T* ptr = reinterpret_cast<T*>(m_Buffer + aligned);
        m_Offset = aligned + size;
        return ptr;
    }

    void Deallocate(T*, std::size_t) noexcept {
        // Linear allocator doesn't free individual allocations
        // Call Reset() to reclaim all memory
    }

    void Reset() noexcept {
        m_Offset = 0;
    }

    template<typename... Args>
    void Construct(T* ptr, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>) {
        new(ptr) T(std::forward<Args>(args)...);
    }

    void Destroy(T* ptr) noexcept {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            ptr->~T();
        }
    }

private:
    std::byte* m_Buffer;
    std::size_t m_BufferSize;
    std::size_t m_Offset;
};