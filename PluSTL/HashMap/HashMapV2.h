//
// Created by Plutex on 1/12/26.
//

// HashMapV2.h
// High-performance hash map for game engines
// Uses Robin Hood hashing with open addressing for optimal cache locality
// Author: Game Engine Team
// C++20 required

#ifndef GAME_HASH_MAP_H
#define GAME_HASH_MAP_H

#include <cstddef>
#include <cstdint>
#include <utility>
#include <type_traits>
#include "Hashers/Default.h"
#include "Allocators/Default.h"


namespace Plu
{
    // ============================================================================
    // GAME HASH MAP
    // ============================================================================

    template
    <
        typename TKey,
        typename TValue,
        typename THash = DefaultHash<TKey>,
        typename TAllocator = DefaultAllocator<std::pair<const TKey, TValue>>
    >
    class GameHashMap {
    public:
        using KeyType = TKey;
        using MappedType = TValue;
        using ValueType = std::pair<const TKey, TValue>;
        using SizeType = std::size_t;
        using HashType = THash;
        using AllocatorType = TAllocator;

    private:
        // ========================================================================
        // INTERNAL BUCKET STRUCTURE
        // ========================================================================

        // Robin Hood hashing metadata: we store DIB (Distance from Initial Bucket)
        // Packed structure for cache efficiency
        struct alignas(8) Bucket {
            uint8_t dib;           // Distance from ideal bucket (0 = empty, 255 = tombstone)
            bool occupied;         // Is this slot occupied?

            // Storage for key-value pair (uninitialized until constructed)
            alignas(ValueType) unsigned char storage[sizeof(ValueType)];

            ValueType* GetValuePtr() noexcept {
                return reinterpret_cast<ValueType*>(storage);
            }

            const ValueType* GetValuePtr() const noexcept {
                return reinterpret_cast<const ValueType*>(storage);
            }
        };

        // We use a separate allocator for buckets
        using BucketAllocator = DefaultAllocator<Bucket>;

        static constexpr uint8_t EMPTY_DIB = 0;
        static constexpr uint8_t TOMBSTONE_DIB = 255;
        static constexpr float MAX_LOAD_FACTOR = 0.875f; // Robin Hood works well up to ~87.5%
        static constexpr SizeType MIN_CAPACITY = 8;

        Bucket* buckets_;
        SizeType capacity_;
        SizeType size_;
        SizeType tombstones_;

        BucketAllocator bucketAllocator_;
        TAllocator valueAllocator_;
        THash hasher_;

        // ========================================================================
        // INTERNAL HELPERS
        // ========================================================================

        // Probe distance calculation (wrapping)
        SizeType ProbeDistance(SizeType hash, SizeType slotIndex) const noexcept {
            SizeType idealIndex = hash & (capacity_ - 1);
            if (slotIndex >= idealIndex) {
                return slotIndex - idealIndex;
            }
            return capacity_ - idealIndex + slotIndex;
        }

        // Find the bucket for a key (returns nullptr if not found)
        // noexcept: No allocations during lookup!
        Bucket* FindBucket(const TKey& key) const noexcept {
            if (capacity_ == 0) return nullptr;

            SizeType hash = hasher_(key);
            SizeType index = hash & (capacity_ - 1);
            SizeType distance = 0;

            while (true) {
                Bucket& bucket = buckets_[index];

                // Empty slot: not found
                if (bucket.dib == EMPTY_DIB) {
                    return nullptr;
                }

                // Check if this bucket could contain our key
                if (bucket.occupied && bucket.dib == distance) {
                    if (bucket.GetValuePtr()->first == key) {
                        return &bucket;
                    }
                }

                // Robin Hood invariant: if our distance exceeds bucket's DIB, key doesn't exist
                if (distance > bucket.dib) {
                    return nullptr;
                }

                index = (index + 1) & (capacity_ - 1);
                ++distance;
            }
        }

        // Insert with Robin Hood displacement
        void InsertInternal(TKey&& key, TValue&& value, SizeType hash) noexcept {
            SizeType index = hash & (capacity_ - 1);
            SizeType distance = 0;

            // Prepare what we're inserting
            Bucket toInsert;
            toInsert.dib = 0;
            toInsert.occupied = true;
            valueAllocator_.Construct(toInsert.GetValuePtr(),
                                      ValueType(std::move(key), std::move(value)));

            while (true) {
                Bucket& bucket = buckets_[index];

                // Empty slot: place here
                if (bucket.dib == EMPTY_DIB || bucket.dib == TOMBSTONE_DIB) {
                    if (bucket.dib == TOMBSTONE_DIB) {
                        --tombstones_;
                    }
                    toInsert.dib = distance;
                    bucket = toInsert;
                    ++size_;
                    return;
                }

                // Robin Hood: steal from the rich (swap if we're further from home)
                if (distance > bucket.dib) {
                    std::swap(toInsert, bucket);
                    distance = bucket.dib;
                }

                index = (index + 1) & (capacity_ - 1);
                ++distance;
            }
        }

        // Rehash to new capacity
        void Rehash(SizeType newCapacity) noexcept {
            // Allocate new buckets
            Bucket* newBuckets = bucketAllocator_.Allocate(newCapacity);
            if (!newBuckets) return; // Allocation failed, keep old table

            // Initialize all buckets as empty
            for (SizeType i = 0; i < newCapacity; ++i) {
                newBuckets[i].dib = EMPTY_DIB;
                newBuckets[i].occupied = false;
            }

            // Swap to new buckets temporarily
            Bucket* oldBuckets = buckets_;
            SizeType oldCapacity = capacity_;

            buckets_ = newBuckets;
            capacity_ = newCapacity;
            SizeType oldSize = size_;
            size_ = 0;
            tombstones_ = 0;

            // Reinsert all elements
            if (oldBuckets) {
                for (SizeType i = 0; i < oldCapacity; ++i) {
                    if (oldBuckets[i].occupied && oldBuckets[i].dib != TOMBSTONE_DIB) {
                        ValueType* oldValue = oldBuckets[i].GetValuePtr();

                        // Move key and value into new table
                        TKey key = std::move(const_cast<TKey&>(oldValue->first));
                        TValue value = std::move(oldValue->second);

                        SizeType hash = hasher_(key);
                        InsertInternal(std::move(key), std::move(value), hash);

                        // Destroy old
                        valueAllocator_.Destroy(oldValue);
                    }
                }

                bucketAllocator_.Deallocate(oldBuckets, oldCapacity);
            }
        }

        bool ShouldGrow() const noexcept {
            return (size_ + tombstones_) >= static_cast<SizeType>(capacity_ * MAX_LOAD_FACTOR);
        }

    public:
        // ========================================================================
        // ITERATOR
        // ========================================================================

        class Iterator {
        private:
            Bucket* buckets_;
            SizeType capacity_;
            SizeType index_;

            void AdvanceToNext() noexcept {
                while (index_ < capacity_) {
                    if (buckets_[index_].occupied && buckets_[index_].dib != TOMBSTONE_DIB) {
                        return;
                    }
                    ++index_;
                }
            }

        public:
            Iterator(Bucket* buckets, SizeType capacity, SizeType index) noexcept
                : buckets_(buckets), capacity_(capacity), index_(index) {
                AdvanceToNext();
            }

            ValueType& operator*() noexcept {
                return *buckets_[index_].GetValuePtr();
            }

            const ValueType& operator*() const noexcept {
                return *buckets_[index_].GetValuePtr();
            }

            ValueType* operator->() noexcept {
                return buckets_[index_].GetValuePtr();
            }

            const ValueType* operator->() const noexcept {
                return buckets_[index_].GetValuePtr();
            }

            Iterator& operator++() noexcept {
                ++index_;
                AdvanceToNext();
                return *this;
            }

            bool operator==(const Iterator& other) const noexcept {
                return index_ == other.index_;
            }

            bool operator!=(const Iterator& other) const noexcept {
                return index_ != other.index_;
            }
        };

        // ========================================================================
        // CONSTRUCTION / DESTRUCTION
        // ========================================================================

        GameHashMap() noexcept
            : buckets_(nullptr)
            , capacity_(0)
            , size_(0)
            , tombstones_(0)
        {}

        explicit GameHashMap(SizeType reserveSize) noexcept
            : GameHashMap() {
            Reserve(reserveSize);
        }

        ~GameHashMap() noexcept {
            Clear();
            if (buckets_) {
                bucketAllocator_.Deallocate(buckets_, capacity_);
            }
        }

        // No copy (game engines typically don't copy large containers)
        GameHashMap(const GameHashMap&) = delete;
        GameHashMap& operator=(const GameHashMap&) = delete;

        // Move semantics
        GameHashMap(GameHashMap&& other) noexcept
            : buckets_(other.buckets_)
            , capacity_(other.capacity_)
            , size_(other.size_)
            , tombstones_(other.tombstones_)
            , bucketAllocator_(std::move(other.bucketAllocator_))
            , valueAllocator_(std::move(other.valueAllocator_))
            , hasher_(std::move(other.hasher_))
        {
            other.buckets_ = nullptr;
            other.capacity_ = 0;
            other.size_ = 0;
            other.tombstones_ = 0;
        }

        GameHashMap& operator=(GameHashMap&& other) noexcept {
            if (this != &other) {
                Clear();
                if (buckets_) {
                    bucketAllocator_.Deallocate(buckets_, capacity_);
                }

                buckets_ = other.buckets_;
                capacity_ = other.capacity_;
                size_ = other.size_;
                tombstones_ = other.tombstones_;
                bucketAllocator_ = std::move(other.bucketAllocator_);
                valueAllocator_ = std::move(other.valueAllocator_);
                hasher_ = std::move(other.hasher_);

                other.buckets_ = nullptr;
                other.capacity_ = 0;
                other.size_ = 0;
                other.tombstones_ = 0;
            }
            return *this;
        }

        // ========================================================================
        // CAPACITY
        // ========================================================================

        [[nodiscard]] SizeType Size() const noexcept { return size_; }
        [[nodiscard]] SizeType Capacity() const noexcept { return capacity_; }
        [[nodiscard]] bool Empty() const noexcept { return size_ == 0; }

        // Reserve capacity (power of 2 for fast modulo)
        void Reserve(SizeType desiredSize) noexcept {
            if (desiredSize <= capacity_) return;

            // Round up to next power of 2
            SizeType newCapacity = MIN_CAPACITY;
            while (newCapacity < desiredSize) {
                newCapacity *= 2;
            }

            Rehash(newCapacity);
        }

        // Clear all elements (keeps capacity)
        void Clear() noexcept {
            if (!buckets_) return;

            for (SizeType i = 0; i < capacity_; ++i) {
                if (buckets_[i].occupied && buckets_[i].dib != TOMBSTONE_DIB) {
                    valueAllocator_.Destroy(buckets_[i].GetValuePtr());
                }
                buckets_[i].dib = EMPTY_DIB;
                buckets_[i].occupied = false;
            }

            size_ = 0;
            tombstones_ = 0;
        }

        // ========================================================================
        // LOOKUP (NO ALLOCATIONS!)
        // ========================================================================

        // Find element by key
        [[nodiscard]] TValue* Find(const TKey& key) noexcept {
            Bucket* bucket = FindBucket(key);
            return bucket ? &bucket->GetValuePtr()->second : nullptr;
        }

        [[nodiscard]] const TValue* Find(const TKey& key) const noexcept {
            Bucket* bucket = FindBucket(key);
            return bucket ? &bucket->GetValuePtr()->second : nullptr;
        }

        // Check if key exists
        [[nodiscard]] bool Contains(const TKey& key) const noexcept {
            return FindBucket(key) != nullptr;
        }

        // ========================================================================
        // MODIFICATION
        // ========================================================================

        // Insert or update
        void Insert(const TKey& key, const TValue& value) noexcept {
            Insert(TKey(key), TValue(value));
        }

        void Insert(TKey&& key, TValue&& value) noexcept {
            // Check if key exists
            Bucket* existing = FindBucket(key);
            if (existing) {
                // Update existing
                existing->GetValuePtr()->second = std::move(value);
                return;
            }

            // Grow if needed
            if (ShouldGrow()) {
                SizeType newCapacity = (capacity_ == 0) ? MIN_CAPACITY : capacity_ * 2;
                Rehash(newCapacity);
            }

            SizeType hash = hasher_(key);
            InsertInternal(std::move(key), std::move(value), hash);
        }

        // Erase by key
        bool Erase(const TKey& key) noexcept {
            Bucket* bucket = FindBucket(key);
            if (!bucket) return false;

            // Destroy value
            valueAllocator_.Destroy(bucket->GetValuePtr());

            // Mark as tombstone
            bucket->dib = TOMBSTONE_DIB;
            bucket->occupied = false;

            --size_;
            ++tombstones_;

            // If too many tombstones, rehash to clean them up
            if (tombstones_ > capacity_ / 4) {
                Rehash(capacity_);
            }

            return true;
        }

        // Operator[] - returns reference, creates default if missing
        TValue& operator[](const TKey& key) noexcept {
            Bucket* bucket = FindBucket(key);
            if (bucket) {
                return bucket->GetValuePtr()->second;
            }

            // Insert default-constructed value
            Insert(TKey(key), TValue());
            bucket = FindBucket(key);
            return bucket->GetValuePtr()->second;
        }

        // ========================================================================
        // ITERATION
        // ========================================================================

        Iterator begin() noexcept {
            return Iterator(buckets_, capacity_, 0);
        }

        Iterator end() noexcept {
            return Iterator(buckets_, capacity_, capacity_);
        }

        // ========================================================================
        // DEBUG
        // ========================================================================

        #ifdef GAME_HASH_MAP_DEBUG
        void DebugPrint() const noexcept {
            for (SizeType i = 0; i < capacity_; ++i) {
                if (buckets_[i].occupied && buckets_[i].dib != TOMBSTONE_DIB) {
                    // Placeholder: in real engine, use logging system
                }
            }
        }

        float GetLoadFactor() const noexcept {
            return capacity_ > 0 ? static_cast<float>(size_) / capacity_ : 0.0f;
        }

        SizeType GetTombstoneCount() const noexcept {
            return tombstones_;
        }
        #endif
    };
}

#endif // GAME_HASH_MAP_H