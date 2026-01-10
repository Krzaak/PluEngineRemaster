#pragma once

#include <cstddef>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>

/**
 * FastHashMap - Wysokowydajna mapa typu Open Addressing.
 * * Główne cechy:
 * - Linear Probing dla maksymalnej wydajności cache.
 * - Single contiguous block of memory (Klucze, Wartości i Metadane blisko siebie).
 * - Zero alokacji podczas wyszukiwania/odczytu.
 * - Obsługa Custom Allocators.
 */

namespace Plu
{

    // Prosty hash bazowy dla typów prymitywnych
    template<typename T>
    struct DefaultHash {
        std::size_t operator()(const T& key) const noexcept {
            if constexpr (sizeof(T) <= 8) {
                uint64_t x = static_cast<uint64_t>(key);
                x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
                x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
                x = x ^ (x >> 31);
                return static_cast<std::size_t>(x);
            } else {
                return 0; // Wymaga specjalizacji dla złożonych typów
            }
        }
    };

    template<typename T>
    class DefaultAllocator {
    public:
        using ValueType = T;

        [[nodiscard]] T* Allocate(std::size_t count) noexcept {
            return static_cast<T*>(::operator new(count * sizeof(T), std::nothrow));
        }

        void Deallocate(T* ptr, std::size_t /*count*/) noexcept {
            ::operator delete(ptr);
        }

        template<typename... Args>
        void Construct(T* ptr, Args&&... args) noexcept {
            new(ptr) T(std::forward<Args>(args)...);
        }

        void Destroy(T* ptr) noexcept {
            if constexpr (!std::is_trivially_destructible_v<T>) {
                ptr->~T();
            }
        }
    };

    template<typename Key, typename Value,
             typename Hash = DefaultHash<Key>,
             typename Allocator = DefaultAllocator<std::byte>>
    class FastHashMap {
    public:
        using KeyType = Key;
        using ValueType = Value;

        struct Pair {
            Key key;
            Value value;
        };

    private:
        enum class SlotState : uint8_t {
            Empty,
            Occupied,
            Deleted
        };

        struct Slot {
            Pair data;
            SlotState state = SlotState::Empty;
        };

        Slot* m_Slots = nullptr;
        std::size_t m_Capacity = 0;
        std::size_t m_Size = 0;
        Allocator m_Allocator;
        Hash m_Hasher;

        static constexpr float MaxLoadFactor = 0.7f;

    public:
        FastHashMap(std::size_t initialCapacity = 16) {
            Reserve(initialCapacity);
        }

        ~FastHashMap() {
            Clear();
            if (m_Slots) {
                m_Allocator.Deallocate(reinterpret_cast<std::byte*>(m_Slots), m_Capacity * sizeof(Slot));
            }
        }

        // Brak kopiowania w silniku (unikamy ukrytych kosztów)
        FastHashMap(const FastHashMap&) = delete;
        FastHashMap& operator=(const FastHashMap&) = delete;

        FastHashMap(FastHashMap&& other) noexcept
            : m_Slots(other.m_Slots), m_Capacity(other.m_Capacity), m_Size(other.m_Size) {
            other.m_Slots = nullptr;
            other.m_Size = 0;
            other.m_Capacity = 0;
        }

        void Reserve(std::size_t newCapacity) {
            if (newCapacity <= m_Capacity) return;

            // Potęga dwójki ułatwia modulo (capacity - 1)
            std::size_t realCap = 16;
            while (realCap < newCapacity) realCap <<= 1;

            Slot* oldSlots = m_Slots;
            std::size_t oldCap = m_Capacity;

            m_Slots = reinterpret_cast<Slot*>(m_Allocator.Allocate(realCap * sizeof(Slot)));
            m_Capacity = realCap;
            m_Size = 0;

            for (std::size_t i = 0; i < m_Capacity; ++i) {
                m_Slots[i].state = SlotState::Empty;
            }

            if (oldSlots) {
                for (std::size_t i = 0; i < oldCap; ++i) {
                    if (oldSlots[i].state == SlotState::Occupied) {
                        Insert(std::move(oldSlots[i].data.key), std::move(oldSlots[i].data.value));
                        if constexpr (!std::is_trivially_destructible_v<Pair>) {
                            oldSlots[i].data.~Pair();
                        }
                    }
                }
                m_Allocator.Deallocate(reinterpret_cast<std::byte*>(oldSlots), oldCap * sizeof(Slot));
            }
        }

        bool Insert(const Key& key, Value&& value) {
            if ((float)(m_Size + 1) > (float)m_Capacity * MaxLoadFactor) {
                Reserve(m_Capacity * 2);
            }

            std::size_t h = m_Hasher(key) & (m_Capacity - 1);

            while (m_Slots[h].state == SlotState::Occupied) {
                if (m_Slots[h].data.key == key) return false; // Już istnieje
                h = (h + 1) & (m_Capacity - 1);
            }

            new (&m_Slots[h].data) Pair{ key, std::forward<Value>(value) };
            m_Slots[h].state = SlotState::Occupied;
            m_Size++;
            return true;
        }

        Value* Find(const Key& key) noexcept {
            if (m_Capacity == 0) return nullptr;

            std::size_t h = m_Hasher(key) & (m_Capacity - 1);
            std::size_t start = h;

            while (m_Slots[h].state != SlotState::Empty) {
                if (m_Slots[h].state == SlotState::Occupied && m_Slots[h].data.key == key) {
                    return &m_Slots[h].data.value;
                }
                h = (h + 1) & (m_Capacity - 1);
                if (h == start) break;
            }
            return nullptr;
        }

        bool Erase(const Key& key) {
            std::size_t h = m_Hasher(key) & (m_Capacity - 1);
            std::size_t start = h;

            while (m_Slots[h].state != SlotState::Empty) {
                if (m_Slots[h].state == SlotState::Occupied && m_Slots[h].data.key == key) {
                    if constexpr (!std::is_trivially_destructible_v<Pair>) {
                        m_Slots[h].data.~Pair();
                    }
                    m_Slots[h].state = SlotState::Deleted;
                    m_Size--;
                    return true;
                }
                h = (h + 1) & (m_Capacity - 1);
                if (h == start) break;
            }
            return false;
        }

        void Clear() {
            if constexpr (!std::is_trivially_destructible_v<Pair>) {
                for (std::size_t i = 0; i < m_Capacity; ++i) {
                    if (m_Slots[i].state == SlotState::Occupied) {
                        m_Slots[i].data.~Pair();
                    }
                }
            }
            for (std::size_t i = 0; i < m_Capacity; ++i) m_Slots[i].state = SlotState::Empty;
            m_Size = 0;
        }

        std::size_t Size() const { return m_Size; }
    };

} // namespace Engine