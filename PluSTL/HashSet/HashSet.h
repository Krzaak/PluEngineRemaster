//
// Created by Plutex on 1/16/26.
//

#ifndef PLUENGINE_HASHSET_H
#define PLUENGINE_HASHSET_H

#include "Allocators/Default.h"
#include "Hashers/Default.h"

namespace Plu
{
	// ============================================================================
    // HashSet - Open addressing hash set with linear probing
    // ============================================================================
    template
    <
        typename T,
        typename Hasher = Plu::DefaultHash<T>,
        typename Allocator = DefaultAllocator<T>
    >
    class HashSet {
    private:
        // Stan slotu w tablicy hash
        enum class SlotState : unsigned char {
            Empty,    // Nigdy nie używany
            Occupied, // Zawiera wartość
            Deleted   // Był używany, ale wartość została usunięta
        };

        // Pojedynczy slot w tablicy hash
        struct Slot {
            alignas(T) unsigned char Storage[sizeof(T)]; // Wyrównane storage dla T
            SlotState State;

            Slot() noexcept : State(SlotState::Empty) {}

            T* GetValue() noexcept {
                return reinterpret_cast<T*>(Storage);
            }

            const T* GetValue() const noexcept {
                return reinterpret_cast<const T*>(Storage);
            }
        };

        Slot* mSlots;              // Tablica slotów
        std::size_t mCapacity;     // Pojemność tablicy
        std::size_t mSize;         // Liczba elementów
        std::size_t mDeletedCount; // Liczba usuniętych slotów
        Hasher mHasher;            // Funkcja hashująca
        Allocator mAllocator;      // Alokator

        static constexpr std::size_t DEFAULT_CAPACITY = 16;
        static constexpr float MAX_LOAD_FACTOR = 0.75f;
        static constexpr float REHASH_THRESHOLD = 0.5f; // Próg deleted/capacity dla rehash

        // Alokator dla Slot (używamy tego samego alokatora, ale konwertujemy typ)
        using SlotAllocator = DefaultAllocator<Slot>;
        SlotAllocator mSlotAllocator;

    public:
        // ========================================================================
        // Iteratory
        // ========================================================================
        class Iterator {
            friend class HashSet;

            Slot* mCurrent;
            Slot* mEnd;

            Iterator(Slot* current, Slot* end) noexcept
                : mCurrent(current), mEnd(end) {
                // Skip do pierwszego occupied slotu
                while (mCurrent != mEnd && mCurrent->State != SlotState::Occupied) {
                    ++mCurrent;
                }
            }

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;

            T& operator*() const noexcept {
                return *mCurrent->GetValue();
            }

            T* operator->() const noexcept {
                return mCurrent->GetValue();
            }

            Iterator& operator++() noexcept {
                ++mCurrent;
                while (mCurrent != mEnd && mCurrent->State != SlotState::Occupied) {
                    ++mCurrent;
                }
                return *this;
            }

            Iterator operator++(int) noexcept {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const Iterator& other) const noexcept {
                return mCurrent == other.mCurrent;
            }

            bool operator!=(const Iterator& other) const noexcept {
                return mCurrent != other.mCurrent;
            }
        };

        class ConstIterator {
            friend class HashSet;

            const Slot* mCurrent;
            const Slot* mEnd;

            ConstIterator(const Slot* current, const Slot* end) noexcept
                : mCurrent(current), mEnd(end) {
                while (mCurrent != mEnd && mCurrent->State != SlotState::Occupied) {
                    ++mCurrent;
                }
            }

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;

            const T& operator*() const noexcept {
                return *mCurrent->GetValue();
            }

            const T* operator->() const noexcept {
                return mCurrent->GetValue();
            }

            ConstIterator& operator++() noexcept {
                ++mCurrent;
                while (mCurrent != mEnd && mCurrent->State != SlotState::Occupied) {
                    ++mCurrent;
                }
                return *this;
            }

            ConstIterator operator++(int) noexcept {
                ConstIterator tmp = *this;
                ++(*this);
                return tmp;
            }

            bool operator==(const ConstIterator& other) const noexcept {
                return mCurrent == other.mCurrent;
            }

            bool operator!=(const ConstIterator& other) const noexcept {
                return mCurrent != other.mCurrent;
            }
        };

        // ========================================================================
        // Konstruktory i destruktor
        // ========================================================================
        HashSet() noexcept
            : mSlots(nullptr)
            , mCapacity(0)
            , mSize(0)
            , mDeletedCount(0) {
            Reserve(DEFAULT_CAPACITY);
        }

        explicit HashSet(std::size_t initialCapacity) noexcept
            : mSlots(nullptr)
            , mCapacity(0)
            , mSize(0)
            , mDeletedCount(0) {
            Reserve(initialCapacity);
        }

        ~HashSet() noexcept {
            Clear();
            if (mSlots) {
                mSlotAllocator.Deallocate(mSlots, mCapacity);
            }
        }

        // Copy constructor
        HashSet(const HashSet& other) noexcept
            : mSlots(nullptr)
            , mCapacity(0)
            , mSize(0)
            , mDeletedCount(0)
            , mHasher(other.mHasher) {
            Reserve(other.mCapacity);
            for (std::size_t i = 0; i < other.mCapacity; ++i) {
                if (other.mSlots[i].State == SlotState::Occupied) {
                    Insert(*other.mSlots[i].GetValue());
                }
            }
        }

        // Move constructor
        HashSet(HashSet&& other) noexcept
            : mSlots(other.mSlots)
            , mCapacity(other.mCapacity)
            , mSize(other.mSize)
            , mDeletedCount(other.mDeletedCount)
            , mHasher(std::move(other.mHasher)) {
            other.mSlots = nullptr;
            other.mCapacity = 0;
            other.mSize = 0;
            other.mDeletedCount = 0;
        }

        // Copy assignment
        HashSet& operator=(const HashSet& other) noexcept {
            if (this != &other) {
                Clear();
                mHasher = other.mHasher;
                Reserve(other.mCapacity);
                for (std::size_t i = 0; i < other.mCapacity; ++i) {
                    if (other.mSlots[i].State == SlotState::Occupied) {
                        Insert(*other.mSlots[i].GetValue());
                    }
                }
            }
            return *this;
        }

        // Move assignment
        HashSet& operator=(HashSet&& other) noexcept {
            if (this != &other) {
                Clear();
                if (mSlots) {
                    mSlotAllocator.Deallocate(mSlots, mCapacity);
                }

                mSlots = other.mSlots;
                mCapacity = other.mCapacity;
                mSize = other.mSize;
                mDeletedCount = other.mDeletedCount;
                mHasher = std::move(other.mHasher);

                other.mSlots = nullptr;
                other.mCapacity = 0;
                other.mSize = 0;
                other.mDeletedCount = 0;
            }
            return *this;
        }

        // ========================================================================
        // Główne operacje
        // ========================================================================

        // Insert z const reference
        bool Insert(const T& value) noexcept {
            return InsertInternal(value);
        }

        // Insert z move semantics
        bool Insert(T&& value) noexcept {
            return InsertInternal(std::move(value));
        }

        // Remove element
        bool Remove(const T& value) noexcept {
            if (mCapacity == 0) return false;

            std::size_t index = FindSlotIndex(value);
            Slot& slot = mSlots[index];

            if (slot.State != SlotState::Occupied) {
                return false;
            }

            // Zniszcz wartość i oznacz jako deleted
            mAllocator.Destroy(slot.GetValue());
            slot.State = SlotState::Deleted;
            --mSize;
            ++mDeletedCount;

            // Rehash jeśli za dużo deleted slotów
            if (mDeletedCount > mCapacity * REHASH_THRESHOLD) {
                Rehash(mCapacity);
            }

            return true;
        }

        // Find element (returns iterator)
        Iterator Find(const T& value) noexcept {
            if (mCapacity == 0) {
                return end();
            }

            std::size_t index = FindSlotIndex(value);
            if (mSlots[index].State == SlotState::Occupied) {
                return Iterator(&mSlots[index], mSlots + mCapacity);
            }
            return end();
        }

        ConstIterator Find(const T& value) const noexcept {
            if (mCapacity == 0) {
                return cend();
            }

            std::size_t index = FindSlotIndex(value);
            if (mSlots[index].State == SlotState::Occupied) {
                return ConstIterator(&mSlots[index], mSlots + mCapacity);
            }
            return cend();
        }

        // Contains check
        bool Contains(const T& value) const noexcept {
            return Find(value) != cend();
        }

        // Clear all elements
        void Clear() noexcept {
            if (!mSlots) return;

            for (std::size_t i = 0; i < mCapacity; ++i) {
                if (mSlots[i].State == SlotState::Occupied) {
                    mAllocator.Destroy(mSlots[i].GetValue());
                }
                mSlots[i].State = SlotState::Empty;
            }
            mSize = 0;
            mDeletedCount = 0;
        }

        // Reserve capacity
        void Reserve(std::size_t newCapacity) noexcept {
            if (newCapacity <= mCapacity) return;

            // Zaokrąglij do potęgi dwójki dla lepszej wydajności
            newCapacity = NextPowerOfTwo(newCapacity);
            Rehash(newCapacity);
        }

        // Rehash with new capacity
        void Rehash(std::size_t newCapacity) noexcept {
            if (newCapacity < mSize) {
                newCapacity = NextPowerOfTwo(mSize * 2);
            }

            Slot* oldSlots = mSlots;
            std::size_t oldCapacity = mCapacity;

            // Alokuj nową tablicę
            mSlots = mSlotAllocator.Allocate(newCapacity);
            if (!mSlots) {
                mSlots = oldSlots; // Przywróć stary wskaźnik w razie błędu
                return;
            }

            // Zainicjalizuj nowe sloty
            for (std::size_t i = 0; i < newCapacity; ++i) {
                mSlotAllocator.Construct(&mSlots[i]);
            }

            mCapacity = newCapacity;
            std::size_t oldSize = mSize;
            mSize = 0;
            mDeletedCount = 0;

            // Przenieś stare wartości
            if (oldSlots) {
                for (std::size_t i = 0; i < oldCapacity; ++i) {
                    if (oldSlots[i].State == SlotState::Occupied) {
                        // Move wartość do nowej tablicy
                        InsertInternal(std::move(*oldSlots[i].GetValue()));
                        mAllocator.Destroy(oldSlots[i].GetValue());
                    }
                }
                mSlotAllocator.Deallocate(oldSlots, oldCapacity);
            }
        }

        // ========================================================================
        // Gettery
        // ========================================================================
        std::size_t Size() const noexcept { return mSize; }
        std::size_t Capacity() const noexcept { return mCapacity; }
        bool IsEmpty() const noexcept { return mSize == 0; }
        float LoadFactor() const noexcept {
            return mCapacity > 0 ? static_cast<float>(mSize) / mCapacity : 0.0f;
        }

        // ========================================================================
        // Iteratory
        // ========================================================================
        Iterator begin() noexcept {
            return Iterator(mSlots, mSlots + mCapacity);
        }

        Iterator end() noexcept {
            return Iterator(mSlots + mCapacity, mSlots + mCapacity);
        }

        ConstIterator begin() const noexcept {
            return ConstIterator(mSlots, mSlots + mCapacity);
        }

        ConstIterator end() const noexcept {
            return ConstIterator(mSlots + mCapacity, mSlots + mCapacity);
        }

        ConstIterator cbegin() const noexcept {
            return ConstIterator(mSlots, mSlots + mCapacity);
        }

        ConstIterator cend() const noexcept {
            return ConstIterator(mSlots + mCapacity, mSlots + mCapacity);
        }

    private:
        // ========================================================================
        // Wewnętrzne metody pomocnicze
        // ========================================================================

        // Znajdź indeks slotu dla wartości (linear probing)
        std::size_t FindSlotIndex(const T& value) const noexcept {
            std::size_t hash = mHasher(value);
            std::size_t index = hash & (mCapacity - 1); // Szybki modulo dla potęg 2
            std::size_t firstDeleted = mCapacity;

            // Linear probing
            for (std::size_t i = 0; i < mCapacity; ++i) {
                std::size_t probeIndex = (index + i) & (mCapacity - 1);
                const Slot& slot = mSlots[probeIndex];

                if (slot.State == SlotState::Empty) {
                    // Jeśli znaleźliśmy deleted slot wcześniej, użyj go
                    return firstDeleted != mCapacity ? firstDeleted : probeIndex;
                }

                if (slot.State == SlotState::Deleted) {
                    if (firstDeleted == mCapacity) {
                        firstDeleted = probeIndex;
                    }
                    continue;
                }

                // Porównaj wartości
                if (*slot.GetValue() == value) {
                    return probeIndex;
                }
            }

            return firstDeleted != mCapacity ? firstDeleted : index;
        }

        // Wewnętrzna implementacja insert z perfect forwarding
        template<typename U>
        bool InsertInternal(U&& value) noexcept {
            // Sprawdź czy trzeba zwiększyć capacity
            if (mSize + 1 > mCapacity * MAX_LOAD_FACTOR) {
                Reserve(mCapacity * 2);
            }

            std::size_t index = FindSlotIndex(value);
            Slot& slot = mSlots[index];

            // Element już istnieje
            if (slot.State == SlotState::Occupied) {
                return false;
            }

            // Wstaw nowy element
            if (slot.State == SlotState::Deleted) {
                --mDeletedCount;
            }

            mAllocator.Construct(slot.GetValue(), std::forward<U>(value));
            slot.State = SlotState::Occupied;
            ++mSize;

            return true;
        }

        // Oblicz następną potęgę dwójki
        static std::size_t NextPowerOfTwo(std::size_t n) noexcept {
            if (n <= 1) return 1;
            --n;
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            n |= n >> 16;
            n |= n >> 32;
            return n + 1;
        }
    };
}

#endif //PLUENGINE_HASHSET_H