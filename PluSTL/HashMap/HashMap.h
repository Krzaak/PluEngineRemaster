//
// Created by Plutex on 1/4/26.
//

#ifndef PLUENGINE_HASHMAP_H
#define PLUENGINE_HASHMAP_H

#include <cstddef>
#include <cstdint>
#include <utility>
#include <type_traits>
#include <bit>

namespace Plu
{
    // ============================================================================
    // HASH MAP IMPLEMENTATION
    // ============================================================================

    template<
        typename Key,
        typename Value,
        typename Hasher = std::hash<Key>,
        typename KeyEqual = std::equal_to<Key>,
        typename Allocator = DefaultAllocator<std::pair<Key, Value>>
    >
    class GameHashMap {
    public:
        using KeyType = Key;
        using MappedType = Value;
        using ValueType = std::pair<Key, Value>;
        using SizeType = std::size_t;
        using HasherType = Hasher;
        using KeyEqualType = KeyEqual;
        using AllocatorType = Allocator;

        // ========================================================================
        // INTERNAL STRUCTURES
        // ========================================================================

    private:
        // Robin Hood hashing requires distance-from-ideal-bucket (DIB) tracking
        struct Bucket {
            ValueType Data;
            std::uint32_t Hash;      // Cached hash for faster lookups
            std::int32_t DIB;        // Distance from ideal bucket (-1 = empty)

            Bucket() noexcept : Hash(0), DIB(-1) {}

            [[nodiscard]] bool IsOccupied() const noexcept { return DIB >= 0; }
            [[nodiscard]] bool IsEmpty() const noexcept { return DIB < 0; }
        };

        static constexpr float DEFAULT_MAX_LOAD_FACTOR = 0.85f;
        static constexpr SizeType DEFAULT_CAPACITY = 16;

        Bucket* m_Buckets;
        SizeType m_Capacity;
        SizeType m_Size;
        float m_MaxLoadFactor;
        Hasher m_Hasher;
        KeyEqual m_KeyEqual;
        [[no_unique_address]] Allocator m_Allocator;

        // ========================================================================
        // HELPER FUNCTIONS
        // ========================================================================

        [[nodiscard]] static SizeType NextPowerOfTwo(SizeType n) noexcept {
            if (n == 0) return 1;
            return SizeType(1) << (64 - std::countl_zero(n - 1));
        }

        [[nodiscard]] SizeType GetIdealBucket(std::uint32_t hash) const noexcept {
            // Fast modulo for power-of-2 sizes
            return hash & (m_Capacity - 1);
        }

        [[nodiscard]] SizeType GetThreshold() const noexcept {
            return static_cast<SizeType>(m_Capacity * m_MaxLoadFactor);
        }

        void AllocateBuckets(SizeType capacity) noexcept {
            // Ensure capacity is power of 2 for fast modulo
            capacity = NextPowerOfTwo(capacity);

            // Allocate using custom allocator
            m_Buckets = m_Allocator.Allocate(capacity);

            if (m_Buckets) {
                m_Capacity = capacity;

                // Initialize all buckets as empty
                for (SizeType i = 0; i < capacity; ++i) {
                    m_Allocator.Construct(&m_Buckets[i]);
                }
            } else {
                m_Capacity = 0;
            }
        }

        void DeallocateBuckets() noexcept {
            if (m_Buckets) {
                // Destroy occupied buckets
                for (SizeType i = 0; i < m_Capacity; ++i) {
                    if (m_Buckets[i].IsOccupied()) {
                        m_Allocator.Destroy(&m_Buckets[i].Data);
                    }
                    m_Allocator.Destroy(&m_Buckets[i]);
                }

                m_Allocator.Deallocate(m_Buckets, m_Capacity);
                m_Buckets = nullptr;
                m_Capacity = 0;
            }
        }

        void Rehash(SizeType newCapacity) noexcept {
            Bucket* oldBuckets = m_Buckets;
            SizeType oldCapacity = m_Capacity;

            m_Buckets = nullptr;
            m_Capacity = 0;
            m_Size = 0;

            AllocateBuckets(newCapacity);

            if (!m_Buckets) {
                // Allocation failed, restore old state
                m_Buckets = oldBuckets;
                m_Capacity = oldCapacity;
                return;
            }

            // Reinsert all elements
            for (SizeType i = 0; i < oldCapacity; ++i) {
                if (oldBuckets[i].IsOccupied()) {
                    InsertInternal(
                        std::move(oldBuckets[i].Data.first),
                        std::move(oldBuckets[i].Data.second),
                        oldBuckets[i].Hash
                    );
                    m_Allocator.Destroy(&oldBuckets[i].Data);
                }
                m_Allocator.Destroy(&oldBuckets[i]);
            }

            m_Allocator.Deallocate(oldBuckets, oldCapacity);
        }

        // Robin Hood hashing: insert and swap with richer buckets
        void InsertInternal(Key&& key, Value&& value, std::uint32_t hash) noexcept {
            SizeType pos = GetIdealBucket(hash);
            std::int32_t dib = 0;

            // Create temporary bucket to insert
            Bucket toInsert;
            m_Allocator.Construct(&toInsert.Data, std::move(key), std::move(value));
            toInsert.Hash = hash;
            toInsert.DIB = dib;

            while (true) {
                Bucket& bucket = m_Buckets[pos];

                if (bucket.IsEmpty()) {
                    // Found empty slot
                    bucket = std::move(toInsert);
                    ++m_Size;
                    return;
                }

                // Robin Hood: steal from the rich (lower DIB)
                if (toInsert.DIB > bucket.DIB) {
                    std::swap(toInsert, bucket);
                }

                // Move to next bucket (linear probing)
                pos = (pos + 1) & (m_Capacity - 1);
                ++toInsert.DIB;
            }
        }

    public:
        // ========================================================================
        // CONSTRUCTORS & DESTRUCTOR
        // ========================================================================

        explicit GameHashMap(
            SizeType initialCapacity = DEFAULT_CAPACITY,
            const Hasher& hasher = Hasher(),
            const KeyEqual& keyEqual = KeyEqual(),
            const Allocator& allocator = Allocator()
        ) noexcept
            : m_Buckets(nullptr)
            , m_Capacity(0)
            , m_Size(0)
            , m_MaxLoadFactor(DEFAULT_MAX_LOAD_FACTOR)
            , m_Hasher(hasher)
            , m_KeyEqual(keyEqual)
            , m_Allocator(allocator)
        {
            AllocateBuckets(initialCapacity);
        }

        ~GameHashMap() noexcept {
            DeallocateBuckets();
        }

        // Move-only for safety (can add copy if needed)
        GameHashMap(const GameHashMap&) = delete;
        GameHashMap& operator=(const GameHashMap&) = delete;

        GameHashMap(GameHashMap&& other) noexcept
            : m_Buckets(other.m_Buckets)
            , m_Capacity(other.m_Capacity)
            , m_Size(other.m_Size)
            , m_MaxLoadFactor(other.m_MaxLoadFactor)
            , m_Hasher(std::move(other.m_Hasher))
            , m_KeyEqual(std::move(other.m_KeyEqual))
            , m_Allocator(std::move(other.m_Allocator))
        {
            other.m_Buckets = nullptr;
            other.m_Capacity = 0;
            other.m_Size = 0;
        }

        GameHashMap& operator=(GameHashMap&& other) noexcept {
            if (this != &other) {
                DeallocateBuckets();

                m_Buckets = other.m_Buckets;
                m_Capacity = other.m_Capacity;
                m_Size = other.m_Size;
                m_MaxLoadFactor = other.m_MaxLoadFactor;
                m_Hasher = std::move(other.m_Hasher);
                m_KeyEqual = std::move(other.m_KeyEqual);
                m_Allocator = std::move(other.m_Allocator);

                other.m_Buckets = nullptr;
                other.m_Capacity = 0;
                other.m_Size = 0;
            }
            return *this;
        }

        // ========================================================================
        // CAPACITY
        // ========================================================================

        [[nodiscard]] SizeType Size() const noexcept { return m_Size; }
        [[nodiscard]] SizeType Capacity() const noexcept { return m_Capacity; }
        [[nodiscard]] bool IsEmpty() const noexcept { return m_Size == 0; }
        [[nodiscard]] float LoadFactor() const noexcept {
            return m_Capacity > 0 ? static_cast<float>(m_Size) / m_Capacity : 0.0f;
        }

        void Reserve(SizeType count) noexcept {
            SizeType requiredCapacity = static_cast<SizeType>(count / m_MaxLoadFactor) + 1;
            if (requiredCapacity > m_Capacity) {
                Rehash(requiredCapacity);
            }
        }

        void Clear() noexcept {
            for (SizeType i = 0; i < m_Capacity; ++i) {
                if (m_Buckets[i].IsOccupied()) {
                    m_Allocator.Destroy(&m_Buckets[i].Data);
                    m_Buckets[i].DIB = -1;
                }
            }
            m_Size = 0;
        }

        // ========================================================================
        // LOOKUP (NO ALLOCATIONS)
        // ========================================================================

        [[nodiscard]] Value* Find(const Key& key) noexcept {
            if (m_Capacity == 0) return nullptr;

            const std::uint32_t hash = static_cast<std::uint32_t>(m_Hasher(key));
            SizeType pos = GetIdealBucket(hash);
            std::int32_t dib = 0;

            while (true) {
                Bucket& bucket = m_Buckets[pos];

                if (bucket.IsEmpty()) {
                    return nullptr;
                }

                // Robin Hood early exit: if current bucket has lower DIB, key doesn't exist
                if (bucket.DIB < dib) {
                    return nullptr;
                }

                // Check hash first (fast), then key equality
                if (bucket.Hash == hash && m_KeyEqual(bucket.Data.first, key)) {
                    return &bucket.Data.second;
                }

                pos = (pos + 1) & (m_Capacity - 1);
                ++dib;
            }
        }

        [[nodiscard]] const Value* Find(const Key& key) const noexcept {
            return const_cast<GameHashMap*>(this)->Find(key);
        }

        [[nodiscard]] bool Contains(const Key& key) const noexcept {
            return Find(key) != nullptr;
        }

        // ========================================================================
        // INSERTION
        // ========================================================================

        Value& Insert(const Key& key, const Value& value) noexcept {
            return Insert(Key(key), Value(value));
        }

        Value& Insert(Key&& key, Value&& value) noexcept {
            // Check if we need to grow
            if (m_Size >= GetThreshold()) {
                Rehash(m_Capacity * 2);
            }

            const std::uint32_t hash = static_cast<std::uint32_t>(m_Hasher(key));
            SizeType pos = GetIdealBucket(hash);
            std::int32_t dib = 0;

            while (true) {
                Bucket& bucket = m_Buckets[pos];

                if (bucket.IsEmpty()) {
                    // Found empty slot
                    m_Allocator.Construct(&bucket.Data, std::move(key), std::move(value));
                    bucket.Hash = hash;
                    bucket.DIB = dib;
                    ++m_Size;
                    return bucket.Data.second;
                }

                // Key already exists, update value
                if (bucket.Hash == hash && m_KeyEqual(bucket.Data.first, key)) {
                    bucket.Data.second = std::move(value);
                    return bucket.Data.second;
                }

                // Robin Hood: steal from the rich
                if (dib > bucket.DIB) {
                    // Swap and continue inserting the evicted element
                    std::uint32_t tempHash = bucket.Hash;
                    std::int32_t tempDIB = bucket.DIB;

                    ValueType tempData = std::move(bucket.Data);
                    m_Allocator.Destroy(&bucket.Data);
                    m_Allocator.Construct(&bucket.Data, std::move(key), std::move(value));

                    bucket.Hash = hash;
                    bucket.DIB = dib;

                    key = std::move(tempData.first);
                    value = std::move(tempData.second);
                    hash = tempHash;
                    dib = tempDIB;
                }

                pos = (pos + 1) & (m_Capacity - 1);
                ++dib;
            }
        }

        // operator[] for convenient access (inserts default if not found)
        Value& operator[](const Key& key) noexcept {
            Value* found = Find(key);
            if (found) {
                return *found;
            }
            return Insert(Key(key), Value{});
        }

        Value& operator[](Key&& key) noexcept {
            Value* found = Find(key);
            if (found) {
                return *found;
            }
            return Insert(std::move(key), Value{});
        }

        // ========================================================================
        // DELETION
        // ========================================================================

        bool Erase(const Key& key) noexcept {
            if (m_Capacity == 0) return false;

            const std::uint32_t hash = static_cast<std::uint32_t>(m_Hasher(key));
            SizeType pos = GetIdealBucket(hash);
            std::int32_t dib = 0;

            // Find the key
            while (true) {
                Bucket& bucket = m_Buckets[pos];

                if (bucket.IsEmpty() || bucket.DIB < dib) {
                    return false; // Key not found
                }

                if (bucket.Hash == hash && m_KeyEqual(bucket.Data.first, key)) {
                    // Found the key, destroy it
                    m_Allocator.Destroy(&bucket.Data);
                    bucket.DIB = -1;
                    --m_Size;

                    // Backward shift to maintain Robin Hood invariant
                    SizeType nextPos = (pos + 1) & (m_Capacity - 1);
                    while (m_Buckets[nextPos].IsOccupied() && m_Buckets[nextPos].DIB > 0) {
                        m_Buckets[pos] = std::move(m_Buckets[nextPos]);
                        --m_Buckets[pos].DIB;
                        m_Buckets[nextPos].DIB = -1;

                        pos = nextPos;
                        nextPos = (nextPos + 1) & (m_Capacity - 1);
                    }

                    return true;
                }

                pos = (pos + 1) & (m_Capacity - 1);
                ++dib;
            }
        }

        // ========================================================================
        // DEBUG UTILITIES
        // ========================================================================

        #ifdef GAME_HASH_MAP_DEBUG
        void PrintStats() const noexcept {
            printf("=== GameHashMap Stats ===\n");
            printf("Size: %zu / Capacity: %zu (%.1f%% full)\n",
                   m_Size, m_Capacity, LoadFactor() * 100.0f);

            std::int32_t maxDIB = 0;
            double avgDIB = 0.0;

            for (SizeType i = 0; i < m_Capacity; ++i) {
                if (m_Buckets[i].IsOccupied()) {
                    maxDIB = std::max(maxDIB, m_Buckets[i].DIB);
                    avgDIB += m_Buckets[i].DIB;
                }
            }

            if (m_Size > 0) {
                avgDIB /= m_Size;
            }

            printf("Max DIB: %d, Avg DIB: %.2f\n", maxDIB, avgDIB);
            printf("========================\n");
        }
        #endif
    };
}

#endif //PLUENGINE_HASHMAP_H